#ifndef DBXS_SEQUENCES_H
#define DBXS_SEQUENCES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "wtype.h"

typedef struct
{
  int    curlen;   /* indicates array[curlen-1] is defined */
  int    maxlen;   /* indicates array[maxlen-1] can be used */
  WTYPE* array;
} PrimeArray;

extern int   is_prime(WTYPE x);
extern WTYPE next_prime(WTYPE x);
extern int   expand_primearray_index(PrimeArray* p, int index);
extern int   expand_primearray_value(PrimeArray* p, WTYPE value);
extern int   find_best_pair(WTYPE* basis, int basislen, WTYPE val, int adder, int* a, int* b);

extern int prime_count_lower(WTYPE x);
extern int prime_count_upper(WTYPE x);
extern int prime_count(WTYPE x);

extern WTYPE* sieve_base(WTYPE end);
extern WTYPE* sieve_base23(WTYPE end);
extern WTYPE* sieve_atkins(WTYPE end);

#define SET_ARRAY_BIT(ar,n) \
   ar[(n)/BITS_PER_WORD]  |=  (W_ONE << ((n)%BITS_PER_WORD))
#define XOR_ARRAY_BIT(ar,n) \
   ar[(n)/BITS_PER_WORD]  ^=  (W_ONE << ((n)%BITS_PER_WORD))
#define CLR_ARRAY_BIT(ar,n) \
   ar[(n)/BITS_PER_WORD]  &=  ~(W_ONE << ((n)%BITS_PER_WORD))
#define IS_SET_ARRAY_BIT(ar,n) \
   (ar[(n)/BITS_PER_WORD] & (W_ONE << ((n)%BITS_PER_WORD)) )

#endif
