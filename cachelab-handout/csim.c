/*Lab 4, cachelab
 *Introduction to Computer Systems, m14
 *Andrew ID: haileiy
 *Name: Hailei Yu
 *
 *In this file, I wrote a cache simulator without using malloc(). 
 *
 *The only indispensable data structure is cache line, 
 *Sets are arrays of lines, and caches are arrays of sets. 
 *So caches are two dimensional arrays of lines. 
 *And there is no need to use malloc(). 
 */

#include "cachelab.h"
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
/*buffer size of fgets*/
#define BUF_SIZE 100

/*statistics*/
int hits = 0;
int misses = 0;
int evicts = 0;
/*variables to be extracted from traces*/
unsigned addr = 0;
int size = 0;
char oper;
/*variables to be extracted from command line*/
int flag_v = 0;
int flag_h = 0;
int s = 0;
int E = 0;
int b = 0;
char* filename;
int opt;
/*time stamp*/
int curr_time = 0;

/*simple data structure of cache line*/
struct Line {
	int valid;
	unsigned tag;
	unsigned age;//count how many times the line has been accessed
};

int main(int argc, char** argv){
	while ((opt = getopt(argc, argv, "vhs:E:b:t:")) != -1) {
		switch(opt) {
			case 'v':
				flag_v = 1;
				break;
			case 'h':
				flag_h = 1;
				break;
			case 's':
				s = atoi(optarg);
				break;
			case 'E':
				E = atoi(optarg);
				break;
			case 'b':
				b = atoi(optarg);
				break;
			case 't':
				filename = optarg;
				break;
			default:
				break;
		}
	}
	//print help message
	if (flag_h) {
		printf("Usage: ./csim [-hv] -s <num> -E <num> -b <num> -t <file>\n");
		printf("Options:\n");
		printf("\t-h\tPrint this help message.\n");
		printf("\t-v\tOptional verbose flag.\n");
		printf("\t-s <num>  Number of set index bits.\n");
		printf("\t-E <num>  Number of lines per set.\n");
		printf("\t-b <num>  Number of block offset bits.\n");
		printf("\t-t <file> Trace file.\n");
		printf("\n");
		printf("Examples:\n");
		printf("\tlinux> ./csim -s 4 -E 1 -b 4 -t traces/yi.trace\n");
		printf("\tlinux> ./csim -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
	}
	//compute size of the cache and initialize one
	int S = 1 << s;
	struct Line mycache[S][E];
	//initialize
	for (int i = 0; i < S; ++i) {
		for (int j = 0; j < E; ++j) {
			mycache[i][j].valid = 0;
			mycache[i][j].tag = 0;
			mycache[i][j].age = 0;
		}
	}
	
	//open trace files and extract memory access information
	FILE* f = fopen(filename, "r");
	if (!f) return -1;
	
	char buf[BUF_SIZE];
	while (fgets(buf, BUF_SIZE, f) != NULL) {
		sscanf(buf, " %c %x, %d", &oper, &addr, &size);
		/*verbose flag*/
		if (flag_v) {
			printf("%c %x, %d\n", oper, addr, size);
		}
		switch(oper) {
		/*ignore instruction loads*/
			case 'I':
				break;
			/*Data Modify. The second access must hit*/
			case 'M':
				hits++;/*fall through*/
			/*Other cases*/
			case 'L':
			case 'S':
				/*update current time*/
				curr_time ++;
				/*compute tag and index*/
				int mask_index = ~(~0 << s);
				int set_index = (addr >> b) & mask_index;
				int mask_tag = ~ (~0 << (64 - b - s));	
				int tag = (addr >> (s + b)) & mask_tag;
				int miss_flag = 1;
				for (int i = 0; i < E; ++i) {
					if(mycache[set_index][i].tag == tag){
						if(mycache[set_index][i].valid == 1){
							/*HIT! Set the age to the most recently access time*/
							mycache[set_index][i].age = curr_time;
							hits++;
							miss_flag = 0;/*hit*/
							if(flag_v)printf("hit ");
							break;
						}
					}
				}
				/*misses and evicts*/
				if (miss_flag) {
					misses++;
					/*Search the line with oldest age(LRU).*/
					unsigned to_evict = 0;
					int min_age = mycache[set_index][0].age;
					for (int i = 0; i < E; ++i) {
						if (mycache[set_index][i].age < min_age) {
							min_age = mycache[set_index][i].age;
							to_evict = i;
						}
					}
					/*update information of new block*/
					mycache[set_index][to_evict].age = curr_time;
					mycache[set_index][to_evict].tag = tag;
					if(mycache[set_index][to_evict].valid == 1) {
						if(flag_v)printf("evict ");
						evicts++;
					}
					else {
						mycache[set_index][to_evict].valid = 1;
						if(flag_v)printf("miss ");
					}
				}
				break;
			default:
				break;
		}
	}
	fclose(f);
	//print summary
	printSummary(hits, misses, evicts);
	return 0;
}

