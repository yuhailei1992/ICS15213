/* Name = Hailei Yu
 * Andrew ID = haileiy
 * Email = yuhailei1992@gmail.com
 *
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"
#include "contracts.h"
/*Block sizes for three cases respectively*/
#define STEP_1 8
#define STEP_2 8
#define STEP_3 8

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. The REQUIRES and ENSURES from 15-122 are included
 *     for your convenience. They can be removed if you like.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
	
	int i, j;
	int row, column;
	int r0, r1, r2, r3, r4, r5, r6, r7;//local variables
	/*32 * 32
     *use blocks
	 *Divide matrix A into blocks of 8*8,
     *Then perform transpose in each block.
	 *Notice that we should give the diagonal elements special treat
	 *in order to avoid conflict misses.
	 *Don't know why, but the "if (row == column)"line works only when it 
	 * is placed on the current location. May be improved. 
	 */
	if (M == N && N == 32) {
		for(row = 0; row < N; row += STEP_1) {
			for(column = 0; column < M; column += STEP_1) {
				for (i = row; i < row+STEP_1; ++i) {
					for (j = column; j < column+STEP_1; ++j) {
						if (j != i) B[j][i] = A[i][j];
					}
					if (row == column) B[i][i] = A[i][i];
				}
			}
		}
	}
	/*The cache structure is different from 32*32 case,
	 *so we cannot directly use blocks of 8*8.
	 *But when use 4*8 blocks, the cache's potential isn't exploited.
	 *So, simultaneously perform two partial transform operations
	 *to better occupy the cache.
	 */
	else if (M == N && N == 64) {
		for (column = 0; column < M; column += STEP_2) {
			for (row = 0; row < N; row += STEP_2){
				for(i = 0; i < STEP_2; ++i){
					r0 = A[row+i][column+0];
					r1 = A[row+i][column+1];
					r2 = A[row+i][column+2];
					r3 = A[row+i][column+3];
					B[column+0][row+i] = r0;
					B[column+1][row+i] = r1;
					B[column+2][row+i] = r2;
					B[column+3][row+i] = r3;
				}
				for(i = 7; i >= 0; --i){
					r4 = A[row+i][column+4];
					r5 = A[row+i][column+5];
					r6 = A[row+i][column+6];
					r7 = A[row+i][column+7];
					B[column+4][row+i] = r4;
					B[column+5][row+i] = r5;
					B[column+6][row+i] = r6;
					B[column+7][row+i] = r7;
				}
			}
		}
	}
	/*For 61*67
	 *Use the same method as the case of 32*32
	 *to proceed the part of matrix A that could be divided into blocks,
	 *say, the part of 64*56
	 *Then proceed the remaining edge elements.
	 * 
	 *Initially, the miss count was over 2400
	 *By iterately adjusting the value of STEP_3 and the order of 
	 *rows and columns
	 *luckily I reduced the miss count to 1942
	 */
	else if (M == 61 && N == 67) {
		for (column = 0; column < 56; column += STEP_3) {
			for (row = 0; row < 64; row += STEP_3) {
				for (i = row; i < row+STEP_3; ++i) {
					for (j = column; j < column+STEP_3; ++j) {
						B[j][i] = A[i][j];
					}
				}
			}
		}
		for (i = 0; i < 67; ++i) {
			for (j = 56; j < 61; ++j) {
				B[j][i] = A[i][j];
			}
		}
		for (j = 0; j < 56; ++j) {
			for (i = 64; i < 67; ++i) {
				B[j][i] = A[i][j];
			}
		}
	}
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";

void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    REQUIRES(M > 0);
    REQUIRES(N > 0);

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

    ENSURES(is_transpose(M, N, A, B));
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    /*registerTransFunction(trans, trans_desc); */

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

