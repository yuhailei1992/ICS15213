/* CSAPP Lab 6
 * Hailei Yu, haileiy

 * Simple, 64-bit clean allocator based on segregated free
 * lists, first fit placement, and boundary tag coalescing, as described
 * in the CS:APP2e text. Blocks must be aligned to doubleword (8 byte)
 * boundaries. Minimum block size is 16 bytes.
 */

/* =================Design====================
 * Features: segregated list, first fit, 4 byte pointer, 
 * and no footer for allocated blocks

 * ---------------Block design----------------
 * I noticed that the footers of allocated blocks are useless, so I removed it.
 * However, in coalesce, the previous block's free status is needed,
 * so I used the second rightmost bit to store the previous block's free status
 *
 * Allocated block structure:
 * | header(4) | payload | padding(optional) |

 * Free block structure:
 * | hdr(4) | prev_ptr(4) | next_ptr(4) | unused | padding(optional) | ftr(4) |
 *
 * ---------------Segregated list design------
 * header -> free node -> free node -> free node (the list is doubly linked)
 * The headers is additional headers, stored in lower addresses in the heap
 * The 14 headers are initialized to NULL in mm_init
 *
 * ---------------First fit-------------------
 * Nothing special. Given a size, locate the proper header, and search the list
 * until we find a valid free block. If there is no such block in the list, go 
 * on to the next list. If there is no such block in any list, return NULL.
 *
 * ---------------4 byte pointer--------------
 * Since the heap size is limited to 2^32, we only need 4 bytes to store the 
 * prev/next pointer offsets
 * When getting pointer offset, convert it to 8 byte by adding 0x800000000
 * Vice versa, when putting pointers to memory, convert it to 4 byte offsets 
 * by substracting 0x800000000
 *
 * ---------------heap structure--------------
 * | padding | prologue hdr | list0's hdr | ... |list13's hdr | prologue ftr |
 * cont. | blocks | blocks | ... | blocks | epilogue |
 */

/* ===============Misc======================
 * featuring code on CS:APP 2e
 */


#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mm.h"
#include "memlib.h"

/*
 * macros
 */

#define NUM_LISTS   14
#define POINTER_SIZE    4
#define LISTS_LIMIT 0x800000008 + (NUM_LISTS-1)*2*POINTER_SIZE
#define WSIZE       4
#define DSIZE       8
#define CHUNKSIZE  512
/*minimum block size: header=footer=4bytes, prev=post=8bytes*/
#define BLOCKSIZE   2*WSIZE+2*POINTER_SIZE  
#define ALIGNMENT   8
#define ADDI_HEADER_SIZE    DSIZE+2*NUM_LISTS*POINTER_SIZE
#define HEAP_BOTTOM (char *)0x800000000

#ifndef NDEBUG
#define dbg_printf(...) printf(__VA_ARGS__)
#else
#define dbg_printf(...)
#endif

/*
 * Global variables
 */

static char *heap_listp = 0;
static char *list0 = 0;/*points to the first list header in the heap*/

/*
 * inline functions prototypes
 */

static inline unsigned align (size_t p);
static inline unsigned max(unsigned a, unsigned b);
static inline unsigned pack(unsigned size, unsigned alloc);
static inline void put(char *bp, unsigned val);
static inline unsigned get (char *bp);
static inline size_t get_size (char *bp);
static inline int get_alloc (char *bp);
static inline int get_prev_free_status (char *bp);
static inline char *w2p (unsigned b);
static inline unsigned p2w (char *p);
static inline char *header_ptr(char *bp);
static inline char *footer_ptr(char *bp);
static inline char *next_block_ptr(char *bp);
static inline char *prev_block_ptr(char *bp);
static inline char *get_prev_ptr (char *bp);
static inline char *get_next_ptr (char *bp);
static inline void set_prev_ptr (char *bp, char *ptr);
static inline void set_next_ptr (char *bp, char *ptr);
static inline void set_next_header_f (char *bp);
static inline void set_next_header_a (char *bp);

/*
 * inline functions, arithmetic
 */

/*aligns size to meet 8-byte alignment requirement*/
static inline unsigned align (size_t p) {
    if (p < 5) return 1;
    else return (((size_t)(p) + (3)) & ~0x7);
}
/*returns the bigger value*/
static inline unsigned max(unsigned a, unsigned b) {
    return ((a<b)?b:a);
}
/*pack two numbers, use LSBs to store alloc/free information*/
static inline unsigned pack(unsigned size, unsigned alloc) {
    return size | alloc;
}

/*
 * inline functions, data operations (get and put)
 */
/*put a work in the memory*/
static inline void put(char *bp, unsigned val) {
    (*(unsigned int *)(bp) = (val));
}
/*get a word from memory*/
static inline unsigned get (char *bp) {
    return *(unsigned *)(bp);
}
/*get size by masking 3 LSB out*/
static inline size_t get_size (char *bp) {
    return get(bp) & ~0x7;
}
/*getting alloc status by masking the word with 0x1*/
static inline int get_alloc (char *bp) {
    return get(bp) & 0x1;
}
/*getting previous block's free status by masking the word with 0x2*/
static inline int get_prev_free_status (char *bp) {
    return get(header_ptr(bp)) & 0x2;
}

/*
 * inline functions, translation between 8-byte pointers and 4 byte data
 */
/*convert a word to a 8-byte pointer*/
static inline char *w2p (unsigned b) {
    if (b == 0) {
        return NULL;
    }
    else {
        return (char *)((unsigned long)b+0x800000000);
    }
}
/*convert a 8-byte pointer to a word*/
static inline unsigned p2w (char *p) {
    if (!p) {
        return (unsigned)0;
    }
    else {
        return (unsigned)((unsigned long)p-0x800000000);
    }
}

/*
 * inline functions, pointer arithmetic
 */

/* Given block ptr bp, compute address of its header and footer */
static inline char *header_ptr(char *bp) {
    return ((char*)bp - WSIZE);
}
static inline char *footer_ptr(char *bp) {
    return ((char *)(bp) + get_size(header_ptr(bp)) - DSIZE);
}
/* Given block ptr bp, compute address of next and previous blocks */
static inline char *next_block_ptr(char *bp) {
    return ((char *)(bp) + get_size((char *)(bp) - WSIZE));
}
/*prev_block_ptr can be used only when the previous block is free*/
static inline char *prev_block_ptr(char *bp) {
    return ((char *)(bp) - get_size((char *)(bp) - DSIZE));
}
/* Given block ptr bp, compute address of next and previous free blocks */
static inline char *get_prev_ptr (char *bp) {
    unsigned val = get(bp);
    return w2p(val);
}

static inline char *get_next_ptr (char *bp) {
    bp = (char *)bp + POINTER_SIZE;
    unsigned val = get(bp);
    return w2p(val);
}
/* Given block ptr bp, set its field of next and previous free blocks */
static inline void set_prev_ptr (char *bp, char *ptr) {
    put(bp, p2w(ptr));
}

static inline void set_next_ptr (char *bp, char *ptr) {
    put((char *)bp + POINTER_SIZE, p2w(ptr));
}

/*next_block_ptr can be used in any case;
 *but prev_block_ptr can be used only when the previous block is free
 *because allocated blocks have no footer
 */
/* Given block ptr bp, store this block's free status in next block's header */
static inline void set_next_header_f (char *bp) {
    char *next_header = header_ptr(next_block_ptr(bp));
    put(next_header, (get(next_header) & 0xFFFFFFFD));
}

static inline void set_next_header_a (char *bp) {
    char *next_header = header_ptr(next_block_ptr(bp));
    put(next_header, (get(next_header) | 0x2));
}


/*
 * Function prototypes
 */


static char *getListHeader (size_t size);
static char *getNextHeader (char *curr);
static void list_insert(char *bp, size_t size);
static void list_remove(char *bp);
static void printlist();

static void *coalesce(void *bp);
static void place(void *bp, size_t asize);
static void *extend_heap(size_t words);
static void *find_fit(size_t asize);

static void printblock(void *bp);
static void checkblock(void *bp);
static void checkheap(int verbose);
static void checklist();


/*
 * Helper functions
 */
/*get the proper list header by size*/
static char *getListHeader (size_t size) {
    if (size < 16) {
        return (char *)list0;
    }
    else if (size < 32) {
        return (char *)(list0 + 1*2*POINTER_SIZE);
    }
    else if (size < 64) {
        return (char *)(list0 + 2*2*POINTER_SIZE);
    }
    else if (size < 128) {
        return (char *)(list0 + 3*2*POINTER_SIZE);
    }
    else if (size < 256) {
        return (char *)(list0 + 4*2*POINTER_SIZE);
    }
    else if (size < 512) {
        return (char *)(list0 + 5*2*POINTER_SIZE);
    }
    else if (size < 1024) {
        return (char *)(list0 + 6*2*POINTER_SIZE);
    }
    else if (size < 2048) {
        return (char *)(list0 + 7*2*POINTER_SIZE);
    }
    else if (size < 8192) {
        return (char *)(list0 + 8*2*POINTER_SIZE);
    }
    else if (size < 16384) {
        return (char *)(list0 + 9*2*POINTER_SIZE);
    }
    else if (size < 25000) {
        return (char *)(list0 + 10*2*POINTER_SIZE);
    }
    else if (size < 32784) {
        return (char *)(list0 + 11*2*POINTER_SIZE);
    }
    else if (size < 65536) {
        return (char *)(list0 + 12*2*POINTER_SIZE);
    }
    else {
        return (char *)(list0 + 13*2*POINTER_SIZE);
    }
}

/*given current list header, return the next list header*/
static char *getNextHeader (char *bp) {
    assert(bp >= (char *)mem_heap_lo() && bp <= (char *)mem_heap_hi());
    return (long)bp < LISTS_LIMIT ? (char *)(bp + 2*POINTER_SIZE) : NULL;
}
/*
 * insert a free block to the lists
 */
static void list_insert (char *bp, size_t size) {
    /*make sure that bp points to somewhere in the heap*/
    assert(bp >= (char *)mem_heap_lo() && bp <= (char *)mem_heap_hi());
    char *root = getListHeader(size);
    /*root points to the additional header, so we should move to the next node*/
    char *temp = get_next_ptr(root);
    set_next_ptr(bp, temp);
    if (get_next_ptr(root) != NULL) {
        set_prev_ptr((get_next_ptr(root)), bp);
    }
    set_next_ptr(root, bp);
    set_prev_ptr(bp, root);
    //mm_checkheap(0);
    //checklist();
}
/*
 * remove a free block from the lists
 */
static void list_remove(char *bp) {
    /*make sure that bp points to somewhere in the heap*/
    assert(bp >= (char *)mem_heap_lo() && bp <= (char *)mem_heap_hi());
    if(get_prev_ptr(bp)) {
        if (get_next_ptr(bp)) {
            set_prev_ptr(get_next_ptr(bp), get_prev_ptr(bp));
        }
        set_next_ptr(get_prev_ptr(bp),  get_next_ptr(bp));
    }
}
/*
 * print the list. Very useful for debugging
 */
static void printlist () {
    dbg_printf("================Showing free list================\n");
    char* temp = list0;
    while (temp) {
        char *p = temp;
        while (1) {
            dbg_printf("%p->", p);
            if (get_next_ptr(p)) {
                p = get_next_ptr(p);
            }
            else break;
        }
        dbg_printf("\n");
        temp = getNextHeader(temp);
    }
    dbg_printf("================================================\n");
    return;
}

/*
 * initialize the heap
 * set padding, prologue header, free list headers, prologue footer, epilogue
 * and then allocate a initial block by calling extending
 */
int mm_init(void) {
    dbg_printf("Initializing... addi header size is %d\n", ADDI_HEADER_SIZE);
    if((heap_listp = mem_sbrk(ADDI_HEADER_SIZE+2*WSIZE)) == NULL) {
        return -1;
    }
    put(heap_listp, 0);
    put(heap_listp + WSIZE, pack(ADDI_HEADER_SIZE, 1));
    /*set additional head nodes*/
    for (int i = 0; i < 2*NUM_LISTS; ++i) {
        put(heap_listp + DSIZE + i*POINTER_SIZE, 0);
    }
    put(heap_listp + ADDI_HEADER_SIZE, pack(ADDI_HEADER_SIZE, 1));
    /*epilogue*/
    put(heap_listp + WSIZE + ADDI_HEADER_SIZE, pack(0, 1));
    list0 = heap_listp + DSIZE;
    heap_listp += DSIZE;
    assert(*list0 == 0);
    set_next_ptr(list0, NULL);
    set_next_header_a(list0);
    if(extend_heap(CHUNKSIZE / WSIZE) == NULL) {
        return -1;
    }
    /*store current block's free status in the next block's header*/
    set_next_header_a(list0);
    //mm_checkheap(0);
    dbg_printf("Init Done\n");
    return 0;
}

/*
 * mm_malloc - Allocate a block with at least size bytes of payload
 */

void *mm_malloc(size_t size) {
    if (size <= 0)
        return NULL;
    size_t asize;      /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char *bp;
    /* Adjust block size to include overhead and alignment reqs. */
    asize = max(align(size) + DSIZE, BLOCKSIZE);
    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {
        dbg_printf("Found fit, placing...\n");
        place(bp, asize);
        return bp;
    }
    else {
        /* No fit found. Get more memory and place the block */
        dbg_printf("No fit found, extend...\n");
        extendsize = max(asize, 128);
        if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
            return NULL;
        place(bp, asize);
        return bp;
    }
}

/*
 * mm_realloc - reallocate a block
 */
void *mm_realloc(void *ptr, size_t size) {
    void *oldptr = ptr;
    size_t newsize = size;
    /*if size is 0, then this is just free*/
    if(newsize == 0) {
        mm_free(oldptr);
        return NULL;
    }

    /* If oldptr is NULL, then this is just malloc. */
    if(oldptr == NULL) {
        return mm_malloc(newsize);
    }

    size_t oldsize = get_size(header_ptr(oldptr));
    if (oldsize <= newsize) {
        char *newptr = mm_malloc(newsize);
        memcpy(newptr, oldptr, oldsize);
        mm_free(oldptr);
        return newptr;
    }

    if (oldsize > newsize) {
        char *newptr = mm_malloc(oldsize);
        memcpy(newptr, oldptr, newsize);
        mm_free(oldptr);
        return newptr;
    }

    return NULL;
}

/*
 * calloc - allocate a block and set 0
 */

void *calloc (size_t nmemb, size_t size) {
    char *p;
    size_t real_size = nmemb * size;
    p = malloc(real_size);
    memset(p, 0, real_size);
    return p;
}

/*
 * mm_free - Free a block
 */
void mm_free(void *bp) {
    if(bp == 0)
        return;
    if (heap_listp == 0) {
        mm_init();
        return;
    }
    size_t size = get_size(header_ptr(bp));
    /*bit_status stores allocate status and the previous block's free status*/
    unsigned bit_status = 0;
    if (get_prev_free_status(bp)) {
        bit_status |= 2;
    }
    /*for free blocks, set the header and footer*/
    put(header_ptr(bp), pack(size, bit_status));
    put(footer_ptr(bp), pack(size, bit_status));
    set_next_header_f(bp);
    coalesce(bp);
}

/*
 * checkheap--check the heap's and free lists' consistency
 */
int mm_checkheap(int verbose) {
    char *bp = heap_listp;

    if (verbose) {
        printf(">>>>>>>>>>>>>>>>>>>BEGIN CHECKHEAP>>>>>>>>>>>>>>>\n");
        printf("Heap (%p):\n", heap_listp);
    }
    /*check prologue header*/
    dbg_printf("Size is %d\n", (int)get_size(header_ptr(heap_listp)));
    if ((get_size(header_ptr(heap_listp)) != ADDI_HEADER_SIZE)) {
        printf("mm_checkheap...Bad prologue header, get_size error\n");
        return 1;
    }
    if (!get_alloc(header_ptr(heap_listp))) {
        printf("Bad prologue header, get_alloc error\n");
        return 2;
    }
    /*check prologue block*/
    checkblock(heap_listp);
    /*check each block*/
    bp = heap_listp;
    for (; get_size(header_ptr(bp)) > 0; bp = next_block_ptr(bp)) {
        if (verbose) {
            printblock(bp);
        }
        checkblock(bp);
    }

    if (verbose)
        printblock(bp);
    /*check epilogue, whose size should be 0 and alloc_bit should be 1*/
    /*also checks heap boundary*/
    if ((get_size(header_ptr(bp)) != 0) || !(get_alloc(header_ptr(bp)))) {
        printf("Bad epilogue header\n");
        return 3;
    }
    if (verbose)
        printf(">>>>>>>>>>>>>>>>>>>>>>>>END>>>>>>>>>>>>>>>>>>>>\n");
    checklist();
    if (verbose)
        checklist();
    printlist();
    return 0;
}

/*
 * coalesce - Boundary tag coalescing. Return ptr to coalesced block
 */

static void *coalesce(void *bp) {
    assert(bp >= mem_heap_lo() && bp <= mem_heap_hi());
    size_t prev_alloc = get_prev_free_status(bp);
    size_t next_alloc = get_alloc(header_ptr(next_block_ptr(bp)));

    size_t size = get_size(header_ptr(bp));

    if (prev_alloc && !next_alloc) {
        /*case 2: coalesce with the next block
         *should change the size of the next block*/
        dbg_printf("Case2\n");
        size += get_size(header_ptr(next_block_ptr(bp)));
        list_remove(next_block_ptr(bp));
        unsigned bit_status = 0;
        if (get_prev_free_status(bp)) {
            bit_status |= 2;
        }
        put(header_ptr(bp), pack(size, bit_status));
        put(footer_ptr(bp), pack(size, bit_status));
    }

    else if (!prev_alloc && next_alloc) {
        /*case 3: coalesce with the previous block*/
        dbg_printf("Case3\n");
        size += get_size(header_ptr(prev_block_ptr(bp)));
        bp = prev_block_ptr(bp);
        list_remove(bp);
        unsigned bit_status = 0;
        if (get_prev_free_status(bp)) {
            bit_status |= 2;
        }
        put(header_ptr(bp), pack(size, bit_status));
        put(footer_ptr(bp), pack(size, bit_status));
    }

    else if (!prev_alloc && !next_alloc) {
        /*coalesce 3 blocks*/
        dbg_printf("Case4\n");
        size += get_size(header_ptr(prev_block_ptr(bp))) +
                get_size(footer_ptr(next_block_ptr(bp)));
        list_remove(prev_block_ptr(bp));
        list_remove(next_block_ptr(bp));
        bp = prev_block_ptr(bp);
        unsigned bit_status = 0;
        if (get_prev_free_status(bp)) {
            bit_status |= 2;
        }
        put(header_ptr(bp), pack(size, bit_status));
        put(footer_ptr(bp), pack(size, bit_status));
    }
    list_insert(bp, size);
    set_next_header_f(bp);
    return bp;
}

/*
 * place - Place block of asize bytes at start of free block bp
 *         and split if remainder would be at least minimum block size
 */

static void place(void *bp, size_t asize) {
    assert(bp >= mem_heap_lo() && bp <= mem_heap_hi());
    size_t csize = get_size(header_ptr(bp));/*get the free block's size*/
    assert(csize >= asize);

    /*if the remaining size is bigger than a valid block*/
    if ((csize - asize) >= BLOCKSIZE) {
        /*first, set the header with asize*/
        unsigned bit_status = 0;
        if (get_prev_free_status(bp)) {
            bit_status |= 2;
        }
        bit_status |= 1;
        put(header_ptr(bp), pack(asize, bit_status));
        set_next_header_a(bp);
        list_remove(bp);
        /*move to the remaining block*/
        bp = next_block_ptr(bp);
        /*set the new block as a block*/
        bit_status = 0;
        bit_status |= 2;
        put(header_ptr(bp), pack(csize-asize, bit_status));
        put(footer_ptr(bp), pack(csize-asize, bit_status));
        set_next_header_f(bp);
        list_insert(bp, (csize - asize));
    }
    /*if the remaining size is not sufficient for use*/
    else {
        unsigned bit_status = 0;
        if (get_prev_free_status(bp)) {
            bit_status |= 2;
        }
        bit_status |= 1;
        put(header_ptr(bp), pack(csize, bit_status));
        set_next_header_a(bp);
        list_remove(bp);
    }
    //mm_checkheap(1);
}

/*
 * extend_heap - Extend heap with free block and return its block pointer
 */

static void *extend_heap(size_t words) {
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;
    unsigned bit_status = 0;
    if (get_prev_free_status(bp)) {
        bit_status |= 2;
    }
    put(header_ptr(bp), pack(size, bit_status));
    put(footer_ptr(bp), pack(size, bit_status));
    put(header_ptr(next_block_ptr(bp)), pack(0, 3));/*epilogue*/
    dbg_printf("after extending, the alloc is %d", get_prev_free_status(bp));
    return coalesce(bp);
}

/*find_fit, first fit placement*/
static void *find_fit(size_t asize) {
    dbg_printf("Finding...\n");
    char *temp = getListHeader(asize);
    dbg_printf("Initial header is %p...\n", temp);
    while (temp) {
        char *p = temp;
        if (get_next_ptr(p)) {
            p = get_next_ptr(p);
        }
        else {
            temp = getNextHeader(temp);
            continue;
        }
        while (p) {
            if (get_size(header_ptr(p)) >= asize) {
                return (char *)p;
            }
            p = get_next_ptr(p);
        }
        if ((long)temp < LISTS_LIMIT) {
            /*move to the next list*/
            temp = (char *)(temp + 2*POINTER_SIZE);
        }
        else {
            return NULL;
        }
    }
    return NULL;
}
/*
 * printblock - print a block's alloc/free, address, size
 */
static void printblock(void *bp) {
    int hsize, halloc;
    checkheap(0);
    hsize = get_size(header_ptr(bp));
    halloc = get_alloc(header_ptr(bp));
    int prev_status = get_prev_free_status(bp);
    if (hsize == 0) {
        printf("%p: EOL\n", bp);
        return;
    }
    printf("%p: header: [%d:%c] prev_free: %d\n", bp,
           hsize, (halloc ? 'a' : 'f'),
           prev_status);
}

/*
 * checkblock - check a block's alignment, size, header and footer consistency,
 * consecutive free blocks, etc
 */
static void checkblock(void *bp) {
    /*check alignment*/
    if ((size_t)bp % 8)
        printf("Error: %p is not doubleword aligned\n", bp);
    /*check block size*/
    if (bp != list0) {/*skip the prologue, whose size field is not used*/
        if ((get_size(header_ptr(bp))) < BLOCKSIZE) {
            printf("Error: %p is smaller than the minimum size\n", bp);
        }
    }
    /*
     * check free blocks' header and footer consistency
     * cannot check allocated blocks because they don't have footers
     */
    if (!get_alloc(header_ptr(bp))) {
        /*
         * free blocks' header and footer should be identical
         * including size, alloc/free, prev block alloc/free
         */
        if (get(header_ptr(bp)) != get(footer_ptr(bp))) {
            printf("Error: Inconsistent free block header and footer\n");
        }
    }
    /*check consecutive free blocks*/
    if (!get_alloc(header_ptr(bp))) {
        if(!get_alloc(header_ptr(next_block_ptr(bp)))) {
            printf("Error: Two consecutive free blocks!\n");
        }
    }
}

/*
 * checklist - check the segregated list. 
 */
static void checklist() {
    char *seglistp = list0;
    unsigned long count1 = 0;
    unsigned long count2 = 0;
    /*check next/prev pointer consistency by one judge*/
    while (seglistp) {
        char *p = seglistp;
        while (p && get_next_ptr(p)) {
            if (p != get_prev_ptr(get_next_ptr(p))) {
                printf("Error, seglist next/prev pointers mismatch\n");
            }
            p = get_next_ptr(p);
            count1++;
        }
        seglistp = getNextHeader(seglistp);
    }
    /* count free blocks by iterating through every block and traversing 
     * free list by pointers and see if they match
     */
    
    char *count_free_p = list0;
    while (count_free_p) {
        count_free_p = next_block_ptr(count_free_p);
        if (!(get_alloc(header_ptr(count_free_p))))
            count2++;
        if (get_size(header_ptr(count_free_p)) == 0) {
            break;
        }
    }
    if (count1 != count2) {
        printf("Error! count by list %lu is inequal to that by block %lu\n",
               count1, count2);
    }
    /*check if pointers points between mem_heap_lo() and mem_heap_hi()*/
    seglistp = list0;
    while (seglistp) {
        char *p = seglistp;
        while (p && get_next_ptr(p)) {
            if ( (p < (char *)mem_heap_lo()) || (p > (char *)mem_heap_hi()) ) {
                printf("Free list pointer points out range\n");
            }
            p = get_next_ptr(p);
        }
        seglistp = getNextHeader(seglistp);
    }
    /*check if all blocks in each list bucket fall within bucket size range*/
    char *listp = list0;
    char *q;
    q = get_next_ptr(listp);
    while (q) {
        unsigned size = get_size(header_ptr(q));
        printf(".");
        if (size >= 16) {
            printf("node %p in list 0 out of range\n", q);
        }
        q = get_next_ptr(q);
    }
    listp = getNextHeader(listp);
    q = get_next_ptr(listp);
    while (q) {
        unsigned size = get_size(header_ptr(q));
        if (size < 16 || size >= 32) {
            printf("node %p in list 1 out of range\n", q);
        }
        q = get_next_ptr(q);
    }
    listp = getNextHeader(listp);
    q = get_next_ptr(listp);
    while (q) {
        unsigned size = get_size(header_ptr(q));
        if (size < 32 || size >= 64) {
            printf("node %p in list 2 out of range\n", q);
        }
        q = get_next_ptr(q);
    }
    listp = getNextHeader(listp);
    q = get_next_ptr(listp);
    while (q) {
        unsigned size = get_size(header_ptr(q));
        if (size < 64 || size >= 128) {
            printf("node %p in list 3 out of range\n", q);
        }
        q = get_next_ptr(q);
    }
    listp = getNextHeader(listp);
    q = get_next_ptr(listp);
    while (q) {
        unsigned size = get_size(header_ptr(q));
        if (size < 128 || size >= 256) {
            printf("node %p in list 4 out of range\n", q);
        }
        q = get_next_ptr(q);
    }
    listp = getNextHeader(listp);
    q = get_next_ptr(listp);
    while (q) {
        unsigned size = get_size(header_ptr(q));
        if (size < 256 || size >= 512) {
            printf("node %p in list 5 out of range\n", q);
        }
        q = get_next_ptr(q);
    }
    listp = getNextHeader(listp);
    q = get_next_ptr(listp);
    while (q) {
        unsigned size = get_size(header_ptr(q));
        if (size < 512 || size >= 1024) {
            printf("node %p in list 6 out of range\n", q);
        }
        q = get_next_ptr(q);
    }
    listp = getNextHeader(listp);
    q = get_next_ptr(listp);
    while (q) {
        unsigned size = get_size(header_ptr(q));
        if (size < 1024 || size >= 2048) {
            printf("node %p in list 7 out of range\n", q);
        }
        q = get_next_ptr(q);
    }
    listp = getNextHeader(listp);
    q = get_next_ptr(listp);
    while (q) {
        unsigned size = get_size(header_ptr(q));
        if (size < 2048 || size >= 8192) {
            printf("node %p in list 8 out of range\n", q);
        }
        q = get_next_ptr(q);
    }
    listp = getNextHeader(listp);
    q = get_next_ptr(listp);
    while (q) {
        unsigned size = get_size(header_ptr(q));
        if (size < 8192 || size >= 16384) {
            printf("node %p in list 9 out of range\n", q);
        }
        q = get_next_ptr(q);
    }
    listp = getNextHeader(listp);
    q = get_next_ptr(listp);
    while (q) {
        unsigned size = get_size(header_ptr(q));
        if (size < 16384 || size >= 25000) {
            printf("node %p in list 10 out of range\n", q);
        }
        q = get_next_ptr(q);
    }
    listp = getNextHeader(listp);
    q = get_next_ptr(listp);
    while (q) {
        unsigned size = get_size(header_ptr(q));
        if (size < 25000 || size >= 32784) {
            printf("node %p in list 11 out of range\n", q);
        }
        q = get_next_ptr(q);
    }
    listp = getNextHeader(listp);
    q = get_next_ptr(listp);
    while (q) {
        unsigned size = get_size(header_ptr(q));
        if (size < 32784 || size >= 65536) {
            printf("node %p in list 12 out of range\n", q);
        }
        q = get_next_ptr(q);
    }
    listp = getNextHeader(listp);
    q = get_next_ptr(listp);
    while (q) {
        unsigned size = get_size(header_ptr(q));
        if (size < 65536) {
            printf("node %p in list 13 out of range\n", q);
        }
        q = get_next_ptr(q);
    }
}
/*
 * checkheap - Minimal check of the heap for consistency
 */
static void checkheap(int verbose) {
    char *bp = heap_listp;

    if (verbose)
        printf("Heap (%p):\n", heap_listp);
    /*check prologue*/
    if ((get_size(header_ptr(heap_listp)) != ADDI_HEADER_SIZE)) {
        printf("Bad prologue header, get_size error\n");
        return;
    }
    if (!get_alloc(header_ptr(heap_listp))) {
        printf("Bad prologue header, get_alloc error\n");
        return;
    }
    checkblock(heap_listp);
    bp = heap_listp;
    for (; get_size(header_ptr(bp)) > 0; bp = next_block_ptr(bp)) {
        if (verbose)
            printblock(bp);
        checkblock(bp);
    }

    if (verbose)
        printblock(bp);
    /*check epilogue, whose size should be 0 and alloc_bit should be 1*/
    if ((get_size(header_ptr(bp)) != 0) || !(get_alloc(header_ptr(bp))))
        printf("Bad epilogue header\n");
}
