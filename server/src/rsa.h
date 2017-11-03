#ifndef __RSA_H__
#define __RSA_H__

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <limits.h>

/**
 * Basic limb type. Note that some calculations rely on unsigned overflow wrap-around of this type.
 * As a result, only unsigned types should be used here, and the RADIX, HALFRADIX above should be
 * changed as necessary. Unsigned integer should probably be the most efficient word type, and this
 * is used by GMP for example.
 */
typedef unsigned int word;

/* Accuracy with which we test for prime numbers using Solovay-Strassen algorithm.
 * 20 Tests should be sufficient for most largish primes */
#define ACCURACY 20

#define FACTOR_DIGITS 100
#define EXPONENT_MAX RAND_MAX
#define BUF_SIZE 1024

/* Initial capacity for a bignum structure. They will flexibly expand but this
 * should be reasonably high to avoid frequent early reallocs */
#define BIGNUM_CAPACITY 20

/* Radix and halfradix. These should be changed if the limb/word type changes */
#define RADIX 4294967296UL
#define HALFRADIX 2147483648UL

#define MAX(a,b) ((a) > (b) ? (a) : (b))

#define BLOCK_SIZE  82
#define BLOCK_LENGTH  21
#define BLOCK_LENGTH_BYTES (int)(BLOCK_LENGTH * sizeof(int) / sizeof(char))

/**
 * Structure for representing multiple precision integers. This is a base "word" LSB
 * representation. In this case the base, word, is 2^32. Length is the number of words
 * in the current representation. Length should not allow for trailing zeros (Things like
 * 000124). The capacity is the number of words allocated for the limb data.
 */
typedef struct _bignum {
  int length;
  int capacity;
  word* data;
} bignum;


int decodeString(char* src, char** des, bignum* exp, bignum* mod);
int encodeString(char* src, char** des, bignum* exp, bignum* mod);
char* decodeStringChar(char* src, char* exp, char* mod);
char* encodeStringChar(char* src, char* exp, char* mod);

char* encodeBytes(char* src, int len, int bytes, bignum* exp, bignum* mod);
char* decodeBytes(char* src, int len, int bytes, bignum* exp, bignum* mod);
char* encodeBytesChar(char* src, int len, int bytes, char* buf, char* exp, char* mod);
char* decodeBytesChar(char* src, int len, int bytes, char* buf, char* exp, char* mod);

void gen_rsa_key(bignum** pub_exp, bignum** pub_mod, bignum** priv_exp, bignum** priv_mod, int* bytes);

int get_encode_info(int len, int bytes, int* pck_num);
int get_decode_info(int len, int bytes, int* pck_num);

char itoc(char i);
void str_inverse(char* str);
bignum* bignum_init();
void bignum_deinit(bignum* b);
int bignum_iszero(bignum* b);
int bignum_isnonzero(bignum* b);
void bignum_copy(bignum* source, bignum* dest);
void bignum_fromstring(bignum* b, char* string);
void bignum_fromint(bignum* b, unsigned int num);
void bignum_print(bignum* b);
char* bignum_tostring(bignum* b);
int bignum_equal(bignum* b1, bignum* b2);
int bignum_greater(bignum* b1, bignum* b2);
int bignum_less(bignum* b1, bignum* b2);
int bignum_geq(bignum* b1, bignum* b2);
int bignum_leq(bignum* b1, bignum* b2);
void bignum_iadd(bignum* source, bignum* add);
void bignum_add(bignum* result, bignum* b1, bignum* b2);
void bignum_isubtract(bignum* source, bignum* add);
void bignum_subtract(bignum* result, bignum* b1, bignum* b2);
void bignum_imultiply(bignum* source, bignum* add);
void bignum_multiply(bignum* result, bignum* b1, bignum* b2);
void bignum_idivide(bignum* source, bignum* div);
void bignum_idivider(bignum* source, bignum* div, bignum* remainder);
void bignum_remainder(bignum* source, bignum *div, bignum* remainder);
void bignum_imodulate(bignum* source, bignum* modulus);
void bignum_divide(bignum* quotient, bignum* remainder, bignum* b1, bignum* b2);
void bignum_modpow(bignum* base, bignum* exponent, bignum* modulus, bignum* result);
void bignum_gcd(bignum* b1, bignum* b2, bignum* result);
void bignum_inverse(bignum* a, bignum* m, bignum* result);
int bignum_jacobi(bignum* ac, bignum* nc);
int solovayPrime(int a, bignum* n);
int probablePrime(bignum* n, int k);
void randPrime(int numDigits, bignum* result);
void randExponent(bignum* phi, int n, bignum* result);
int readFile(FILE* fd, char** buffer, int bytes);
void encode(bignum* m, bignum* e, bignum* n, bignum* result);
void decode(bignum* c, bignum* d, bignum* n, bignum* result);
bignum *encodeMessage(int len, int bytes, char *message, bignum *exponent, bignum *modulus);
char *decodeMessage(int len, int bytes, bignum *cryptogram, bignum *exponent, bignum *modulus);

#endif








