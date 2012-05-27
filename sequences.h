#ifndef DBXS_SEQUENCES_H
#define DBXS_SEQUENCES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "wtype.h"

extern WTYPE get_prime_cache_size(void);
extern WTYPE get_prime_cache(WTYPE n, const unsigned char** sieve);
extern int   is_prime(WTYPE x);
extern WTYPE next_prime(WTYPE x);

extern long prime_count_lower(WTYPE x);
extern long prime_count_upper(WTYPE x);
extern long prime_count(WTYPE x);

extern WTYPE* sieve_erat(WTYPE end);
extern unsigned char* sieve_erat30(WTYPE end);

typedef struct
{
  int    curlen;   /* indicates array[curlen-1] is defined */
  int    maxlen;   /* indicates array[maxlen-1] can be used */
  WTYPE* array;
} PrimeArray;

extern int   expand_primearray_index(PrimeArray* p, int index);
extern int   expand_primearray_value(PrimeArray* p, WTYPE value);
extern int   find_best_pair(WTYPE* basis, int basislen, WTYPE val, int adder, int* a, int* b);



#define SET_ARRAY_BIT(ar,n) \
   ar[(n)/BITS_PER_WORD]  |=  (W_ONE << ((n)%BITS_PER_WORD))
#define XOR_ARRAY_BIT(ar,n) \
   ar[(n)/BITS_PER_WORD]  ^=  (W_ONE << ((n)%BITS_PER_WORD))
#define CLR_ARRAY_BIT(ar,n) \
   ar[(n)/BITS_PER_WORD]  &=  ~(W_ONE << ((n)%BITS_PER_WORD))
#define IS_SET_ARRAY_BIT(ar,n) \
   (ar[(n)/BITS_PER_WORD] & (W_ONE << ((n)%BITS_PER_WORD)) )

static const WTYPE wheel30[] = {1, 7, 11, 13, 17, 19, 23, 29};
static unsigned char nextwheel30[30] = {
    0,  7,  0,  0,  0,  0,  0, 11,  0,  0,  0, 13,  0, 17,  0,
    0,  0, 19,  0, 23,  0,  0,  0, 29,  0,  0,  0,  0,  0,  1  };
static unsigned char masktab30[30] = {
    0,  1,  0,  0,  0,  0,  0,  2,  0,  0,  0,  4,  0,  8,  0,
    0,  0, 16,  0, 32,  0,  0,  0, 64,  0,  0,  0,  0,  0,128  };

static int is_prime_in_sieve(const unsigned char* sieve, WTYPE p) {
  WTYPE d = p/30;
  WTYPE m = p - d*30;
  /* If m isn't part of the wheel, we return 0 */
  return ( (masktab30[m] != 0) && ((sieve[d] & masktab30[m]) == 0) );
}

/* Useful macros for the wheel-30 sieve array */
#define START_DO_FOR_EACH_SIEVE_PRIME(sieve, a, b) \
  { \
    WTYPE p = a; \
    WTYPE d = p/30; \
    WTYPE m = p-d*30; \
    while (masktab30[m] == 0) { m++; if (m==30) { m=0; d++; } } \
    p = d*30 + m; \
    while ( p <= (b) ) { \
      if ((sieve[d] & masktab30[m]) == 0)

#define END_DO_FOR_EACH_SIEVE_PRIME \
      m = nextwheel30[m];  if (m == 1) { d++; } \
      p = d*30+m; \
    } \
  }


#endif
