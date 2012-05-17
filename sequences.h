#ifndef DBXS_SEQUENCES_H
#define DBXS_SEQUENCES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "wtype.h"

#if 0
  #define BITS_PER_WORD (8 * sizeof(WTYPE))
  #define MAXBIT        (BITS_PER_WORD-1)
  #define NWORDS(bits)  ( ((bits)+BITS_PER_WORD-1) / BITS_PER_WORD )
  #define NBYTES(bits)  ( ((bits)+8-1) / 8 )
#endif

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
extern int find_best_pair(WTYPE* basis, int basislen, WTYPE val, int adder, int* a, int* b);

#endif
