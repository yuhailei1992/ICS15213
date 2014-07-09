/* 
 * CS:APP Data Lab 
 * 
 * <Hailei Yu, haileiy>
 * 
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.  
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.

 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting an integer by more
     than the word size.

EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implent floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operators (! ~ & ^ | + << >>)
     that you are allowed to use for your implementation of the function. 
     The max operator count is checked by dlc. Note that '=' is not 
     counted; you may use as many of these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce 
 *      the correct answers.
 */


#endif
/* 
 * evenBits - return word with all even-numbered bits set to 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 8
 *   Rating: 1
 */
int evenBits(void) {//pass
/*0x5 = 0101b, and is the mask that all even bits are set to 1. 
 * left shift the mask, add the result with the original mask, and do the
 * same operation to the result, the you will get 32 bit word with all even
 * bits set to 1
 */
  int mask0 = 0x55;
  int mask1 = mask0 << 8;
  int mask2 = mask1+mask0;
  int mask3 = mask2 << 16;
  int mask = mask3 + mask2;
  return mask;
}
/* 
 * isEqual - return 1 if x == y, and 0 otherwise 
 *   Examples: isEqual(5,5) = 1, isEqual(4,5) = 0
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int isEqual(int x, int y) {//pass
/*if two numbers are equal, the xor of them will be 0*/
  return !(x ^ y);
}
/* 
 * byteSwap - swaps the nth byte and the mth byte
 *  Examples: byteSwap(0x12345678, 1, 3) = 0x56341278
 *            byteSwap(0xDEADBEEF, 0, 2) = 0xDEEFBEAD
 *  You may assume that 0 <= n <= 3, 0 <= m <= 3
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 25
 *  Rating: 2
 */
int byteSwap(int x, int n, int m) {//pass
/*use masks to extract the intended byte, then use shift to move it to the new position*/
  int ff = 0xFF;
  int n8 = n << 3;
  int m8 = m << 3;

  int window_n = ff << n8;
  int window_m = ff << m8;

  int block_n = ~window_n;
  int block_m = ~window_m;

  int n_sect = ((x & window_n) >> n8) & ff;
  int m_sect = ((x & window_m) >> m8) & ff;
  
  int new_n = m_sect << n8;
  int new_m = n_sect << m8;

  int temp = (x & block_n) | new_n;
  int ret = (temp & block_m) | new_m;
  
  return ret;
}
/* 
 * rotateRight - Rotate x to the right by n
 *   Can assume that 0 <= n <= 31
 *   Examples: rotateRight(0x87654321,4) = 0x18765432
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 25
 *   Rating: 3 
 */
int rotateRight(int x, int n) {//pass
/*use masks to get the bits that should have been rotated out, and then shift them to
 * the leftmost position
 */
  int all_f = ~0;
  int n_32 = ~n + 33;
  int mask_1 = all_f << n;
  int mask_2 = ~mask_1;
  int mask_3 = mask_2 << n_32;
  int mask_4 = ~mask_3;
  int part_1 = (x & mask_2) << n_32;
  int part_2 = (x >> n) & mask_4;
  return part_1 | part_2;
}
/* 
 * logicalNeg - implement the ! operator using any of 
 *              the legal operators except !
 *   Examples: logicalNeg(3) = 0, logicalNeg(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4 
 */
int logicalNeg(int x) {//pass
/*if x is not 0, x or -x is negative. So that's all*/
  int sign_1 = x | (~x + 1);
  int sign = (~sign_1) >> 31;
  return sign & 1;
  
}
/* 
 * TMax - return maximum two's complement integer 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
int tmax(void) {//passed all the tests.
/* tmax is the ~ of tmin, which is 0x80000000*/
  return ~(1 << 31);
}
/* 
 * sign - return 1 if positive, 0 if zero, and -1 if negative
 *  Examples: sign(130) = 1
 *            sign(-23) = -1
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 10
 *  Rating: 2
 */
int sign(int x) {//pass
/*if x is negative, x >> 31 will be 0xffffffff, and !!x is 1, x>>31 | !!x is -1. vice versa*/
  int a = x >> 31;
  int b = !!x;
  return a | b;
}
/* 
 * isGreater - if x > y  then return 1, else return 0 
 *   Example: isGreater(4,5) = 0, isGreater(5,4) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isGreater(int x, int y) {//don't question the code, it's magic!
/*x2 and y2 are half of the original values, so x-y will not overflow. 
 * judge the sign of x-y, 
 * and deal with the corner cases
 */
  int mask = 1;
  int x2 = x >> 1;
  int y2 = y >> 1;
  int z = x2 + (~y2 + 1);
  int sign_1 = (z >> 31) | !!z;
  int sign_2 = (mask & x) + ~(mask & y) + 1; 
  sign_1 = sign_1 << 1;
  return (sign_1 + sign_2 + 3) >> 2;
}
/* 
 * subOK - Determine if can compute x-y without overflow
 *   Example: subOK(0x80000000,0x80000000) = 1,
 *            subOK(0x80000000,0x70000000) = 0, 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 20
 *   Rating: 3
 */
int subOK(int x, int y) {//pass
/* overflow: x is positive, y is negative, result is negative
 * or x is neg, y is pos, result is pos
 * so judge the sign using venn diagram
 * and deal with the corner cases
 */
  int sum = x + (~y + 1);
  int x_sign = x >> 31;
  int y_sign = y >> 31;
  int sum_sign = sum >> 31;
  int temp1 = (x_sign ^ y_sign);//should be 0
  int temp2 = (x_sign ^ sum_sign);//should be 0;
  return !(temp1 & temp2);
}
/*
 * satAdd - adds two numbers but when positive overflow occurs, returns
 *          maximum possible value, and when negative overflow occurs,
 *          it returns minimum possible value.
 *   Examples: satAdd(0x40000000,0x40000000) = 0x7fffffff
 *             satAdd(0x80000000,0xffffffff) = 0x80000000
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 30
 *   Rating: 4
 */
int satAdd(int x, int y) {//pass
/* similar to subOK. just judge the signs of x, y and result,
 * then deal with corner cases.
 */
  int sum = x + y;
  int x_sign = x >> 31;
  int y_sign = y >> 31;
  int sum_sign = sum >> 31;
  int overflow =~((x_sign ^ sum_sign) & (y_sign ^ sum_sign));// 0 for overflow, -1 for not overflow
  int mask = ~overflow;
  int sum_1 = sum | mask;
  int toadd = (overflow ^ 1) << 31;
  int thelast  = !(~x_sign) & !overflow;
  return sum_1 + toadd + thelast;
}
/* howManyBits - return the minimum number of bits required to represent x in
 *             two's complement
 *  Examples: howManyBits(12) = 5
 *            howManyBits(298) = 10
 *            howManyBits(-5) = 4
 *            howManyBits(0)  = 1
 *            howManyBits(-1) = 1
 *            howManyBits(0x80000000) = 32
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 90
 *  Rating: 4
 */
int howManyBits(int x) {
/* binary search algorithm
 * first, make x positive if x is negative,
 * then, count the number of 0s to the left of x's binary representation
 * shift right by 16 bits, judge if the result is 0. if so, it means there are no less than 16 0s
 * so cnt += 16;
 * and then 8 4 2 1,
 * then deal with corner cases
 * done
 */
  int cnt = 0;
  int temp;
  int mask;
  int x_bak = x;  
  int magic = x >> 31;
  x = x ^ magic;
  
  temp = x >> 16;
  mask = ~(!temp) + 1;
  mask = ~mask;
  x = x >> (16 & mask);
  cnt = cnt + (16 & mask);

  temp = x >> 8;
  mask = ~(!temp) + 1;
  mask = ~mask;
  x = x >> (8 & mask);
  cnt = cnt + (8 & mask);
  
  temp = x >> 4;
  mask = ~(!temp) + 1;
  mask = ~mask;
  x = x >> (4 & mask);
  cnt = cnt + (4 & mask);

  temp = x >> 2;
  mask = ~(!temp) + 1;
  mask = ~mask;
  x = x >> (2 & mask);
  cnt = cnt + (2 & mask);
  
  temp = x >> 1;
  mask = ~(!temp) + 1;
  mask = ~mask;
  x = x >> (1 & mask);
  cnt = cnt + (1 & mask);
  cnt =cnt + 2;
  cnt = cnt + ~!(x_bak | 0) + 1;
  cnt = cnt + ~!(x_bak ^ ~0) + 1;
  return cnt;
}
/* 
 * float_half - Return bit-level equivalent of expression 0.5*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_half(unsigned uf) {
/* use masks to extract the sign, exp, frac of a fp number
 * if exp is 0xFF, means infinity or nan, just return uf
 * if exp is 0 or 1, according to the table of denormalized and normalized fp numbers
 * shift the temp right by 1, meaning that divide by 2, and if frac's last two bits are 11, there is a round problem, deal with it
 * in general cases, just subtract exp by one indicate half
 */
	int s_mask = 0x80000000;
	int exp_mask = 0x7F800000;
	int frac_mask = 0x007FFFFF;
	
	int s = uf & s_mask;
	int exp = (uf & exp_mask) >> 23;
	int frac = uf & frac_mask;
	int temp = 0;
	int mask_1 = 0x00FFFFFF;
	if (exp == 0xFF) return uf;//inf and nan
	else if (exp == 0 || exp == 1) {
		temp = (uf & mask_1) >> 1;
		if ((frac & 3) == 3) temp ++;
		return s | temp;
	}
	else return s | ((exp - 1) << 23) | frac;
}
/* 
 * float_f2i - Return bit-level equivalent of expression (int) f
 *   for floating point argument f.
 *   Argument is passed as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point value.
 *   Anything out of range (including NaN and infinity) should return
 *   0x80000000u.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
int float_f2i(unsigned uf) {
/*get the part of frac, shift it left to the leftmost position
 * then shift right according to the value of exp
 * note that we should make the 23th bit to 0, to avoid integer's "supplement by 1s" problem
 * at last, give back the 1, which is omitted by the standard of IEEE754
 * thank god the assignment is all over
 */
	int s = uf & 0x80000000;
	int exp = uf & 0x7F800000;
	int frac = uf & 0x007FFFFF;
	int temp = 0;

	exp = (exp >> 23) - 127;
	if (exp >= 31) return 0x80000000u;
	else if (exp < 0) return 0;//it is decimal
	else {
		temp = frac & 0xFF7FFFFF;
		temp = temp << 8;
		temp = temp >> (31 - exp);
		temp = temp | (1 << exp);
		if(s != 0) temp = ~temp + 1;
		return temp;
	}
}

