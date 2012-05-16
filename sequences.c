#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

/********************  functions to support sequences  ********************/

#include "sequences.h"

/********************  primes  ********************/

/*
 * This implementation:
 *   - stores primes in a realloced array
 *   - generates more by repeated next_prime calls
 *
 * Using the same prime storage and find-pair operations, we could speed
 * things up some by using a sieve when we need to generate a reasonably
 * large range.  This would require a bit of working memory (1 bit for each
 * odd number in the range).
 */

/* primes 2,3,5,7,11,13,17,19,23,29,31 as bits */
#define SMALL_PRIMES_MASK W_CONST(2693408940)
/* if (MOD234_MASK >> (n%30)) & 1, n is a multiple of 2, 3, or 5. */
#define MOD235_MASK       W_CONST(1601558397)

static int _is_prime7(WTYPE x)
{
  WTYPE q, i;
  /* Check for small primes */
  q = x/ 7;  if (q< 7) return 1;  if (x==(q* 7)) return 0;
  q = x/11;  if (q<11) return 1;  if (x==(q*11)) return 0;
  q = x/13;  if (q<13) return 1;  if (x==(q*13)) return 0;
  q = x/17;  if (q<17) return 1;  if (x==(q*17)) return 0;
  q = x/19;  if (q<19) return 1;  if (x==(q*19)) return 0;
  q = x/23;  if (q<23) return 1;  if (x==(q*23)) return 0;
  q = x/29;  if (q<29) return 1;  if (x==(q*29)) return 0;
  /* wheel factorization, mod-30 loop */
  i = 31;
  while (1) {
    q = x/i;  if (q<i) return 1;  if (x==(q*i)) return 0;   i += 6;
    q = x/i;  if (q<i) return 1;  if (x==(q*i)) return 0;   i += 4;
    q = x/i;  if (q<i) return 1;  if (x==(q*i)) return 0;   i += 2;
    q = x/i;  if (q<i) return 1;  if (x==(q*i)) return 0;   i += 4;
    q = x/i;  if (q<i) return 1;  if (x==(q*i)) return 0;   i += 2;
    q = x/i;  if (q<i) return 1;  if (x==(q*i)) return 0;   i += 4;
    q = x/i;  if (q<i) return 1;  if (x==(q*i)) return 0;   i += 6;
    q = x/i;  if (q<i) return 1;  if (x==(q*i)) return 0;   i += 2;
  }
  return 1;
}

WTYPE next_prime(WTYPE x)
{
  WTYPE L, k0, n, M;
  static const WTYPE indices[] = {1, 7, 11, 13, 17, 19, 23, 29};
  int index;
  if (x <= 30) {
    /* Alternate impl: a lookup or case statement */
    if (x <  2) return  2;
    if (x <  3) return  3;
    if (x <  5) return  5;
    if (x <  7) return  7;
    if (x < 11) return 11;
    if (x < 13) return 13;
    if (x < 17) return 17;
    if (x < 19) return 19;
    if (x < 23) return 23;
    if (x < 29) return 29;
    return 31;
  }
  x++;
  L = 30;
  k0 = x/L;
  index = 0;   while ((x-k0*L) > indices[index])  index++;
  n = L*k0 + indices[index];
  M = sizeof(indices)/sizeof(indices[0]);
  while (!_is_prime7(n)) {
    if (++index == M) {  k0++; index = 0; }
    n = L*k0 + indices[index];
  }
  return n;
}

int is_prime(WTYPE x)
{
  if (x <= 31)
    return ((SMALL_PRIMES_MASK >> x) & 1);

  WTYPE q, i;
  if ( (MOD235_MASK >> (x%30)) & 1 )
    return 0;

  return _is_prime7(x);
}

/* p->array[index] will be defined if we return non-zero */
int expand_primearray_index(PrimeArray* p, int index)
{
  int i;
  WTYPE curprime;
  assert(p != 0);
  if (index < 8)  index = 8;
  if (p->curlen > index)
    return 1;
  if (p->array == 0) {
    p->array = (WTYPE*) malloc((index+1) * sizeof(WTYPE));
    p->curlen = 0;
    p->maxlen = index+1;
  }
  if (p->maxlen <= index) {
    p->maxlen = index+1024;
    p->array = (WTYPE*) realloc(p->array, p->maxlen * sizeof(WTYPE));
  }
  if (p->array == 0) {
    p->maxlen = 0;
    p->curlen = 0;
    return 0;
  }
  assert(p->maxlen > index);
  assert(p->maxlen > 2);
  assert(p->curlen >= 0);
  assert(p->array != 0);
  if (p->curlen <= 1) {
    p->array[0] = 2;
    p->array[1] = 3;
    p->curlen = 2;
  }
  curprime = p->array[p->curlen-1];
  for (i = p->curlen; i <= index; i++) {
    curprime = next_prime(curprime);
    p->array[i] = curprime;
  }
  p->curlen = index;
}

/* p->array[p->curlen-1] will be >= value */
int expand_primearray_value(PrimeArray* p, WTYPE value)
{
  int res;
  WTYPE curprime;

  if ( (p->array == 0) || (p->curlen == 0) ) {
    res = expand_primearray_index(p, 8);
    if (res == 0)  return 0;
  }
  /* Should use an inequality to be smarter */

  curprime = p->array[p->curlen-1];
  while (curprime < value) {
    if (p->curlen >= p->maxlen) {
      p->maxlen += 1024;
      p->array = (WTYPE*) realloc(p->array, p->maxlen * sizeof(WTYPE));
      if (p->array == 0) {
        p->maxlen = 0;
        return 0;
      }
    }
    curprime = next_prime(curprime);
    p->array[p->curlen++] = curprime;
  }
  assert(p->array[p->curlen-1] >= value);
  return 1;
}


/********************  best pair  ********************/

static int gamma_length(WTYPE n)
{
  int log2 = 0;
  while (n >= ((2 << log2)-1))  log2++;
  return ((2*log2)+1);
}

/* adder is used to modify the stored indices.  A function would be better. */
int find_best_pair(WTYPE* basis, int basislen, WTYPE val, int adder, int* a, int* b)
{
  int maxbasis = 0;
  int bestlen = INT_MAX;
  int i, j;

  assert( (basis != 0) && (a != 0) && (b != 0) && (basislen >= 1) );
  /* Find how far in basis to look */
  while ( ((maxbasis+100) < basislen) && (basis[maxbasis+100] < val) )
    maxbasis += 100;
  while ( ((maxbasis+1) < basislen) && (basis[maxbasis+1] < val) ) 
    maxbasis++;

  i = 0;
  j = maxbasis;
  while (i <= j) {
    WTYPE sum = basis[i] + basis[j];
    if (sum > val) {
      j--;
    } else {
      if (sum == val) {
        int p1 = i + adder;
        int p2 = j - i + adder;
        int glen = gamma_length(p1) + gamma_length(p2);
        /* printf("found %llu+%llu=%llu  pair %d,%d (%d,%d) with length %d\n", basis[i], basis[j], sum, i, j, p1, p2, glen); */
        if (glen < bestlen) {
          *a = p1;
          *b = p2;
          bestlen = glen;
        }
      }
      i++;
    }
  }
  return (bestlen < INT_MAX);
}
