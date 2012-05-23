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
  static const WTYPE L = 30;
  WTYPE k0, n, M;
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

/*
 * Some pi(x) prime_count functions.  prime_count(x) gives an exact number.
 * For large x, this will either be very slow or take a lot of memory, so
 * think very carefully about whether you need the exact answer.
 *
 * prime_count_lower(x) and prime_count_upper(x) give lower and upper limits,
 * which will bound the exact value.  These bounds should be fairly tight.
 *
 *    pi_upper(x) - pi(x)                     pi_lower(x) - pi(x)
 *    <    10   for x <        5_371          <    10   for x <        9_437
 *    <    50   for x <      295_816          <    50   for x <      136_993
 *    <   100   for x <    1_737_361          <   100   for x <      909_911
 *    <   200   for x <    4_407_916          <   200   for x <    2_274_709
 *    <   400   for x <                       <   400   for x <
 *
 * The upper-lower range is less than 10_000 for all x < 85_849_269.
 *
 * It is common to use the following Chebyshev inequality for x >= 17:
 *    1*x/logx <-> 1.25506*x/logx
 * but this gives terribly loose bounds.
 *
 * Rosser and Schoenfeld's bound for x >= 67 of
 *    x/(logx-1/2) <-> x/(logx-3/2)
 * is much tighter.  These bounds can be tightened even more.
 *
 * The formulas of Dusart for higher x are better yet.  I recommend the paper
 * by Burde for further information.
 *
 * I have applied some tightening to the bounds for small numbers.  These
 * bounds are verified via trial.  The Dusart bounds are used for larger
 * numbers.
 *
 */

static const int prime_count_small[] = 
  {0,0,1,2,2,3,3,4,4,4,4,5,5,6,6,6,6,7,7,8,8,8,8,9,9,9,9,9,9,10,10,
   11,11,11,11,11,11,12,12,12,12,13,13,14,14,14,14,15,15,15,15,15,15,
   16,16,16,16,16,16,17,17,18,18,18,18,18,18,19};
#define NPRIME_COUNT_SMALL  (sizeof(prime_count_small)/sizeof(prime_count_small[0]))
static const float F1 = 1.0;
int prime_count_lower(WTYPE x)
{
  float fx;
  float flogx;
  float bound;

  if (x < NPRIME_COUNT_SMALL)
    return prime_count_small[x];

  fx = (float)x;
  flogx = logf(x);
    /*
       if (x >= l) bound = (fx/flogx) * (F1 + F1/flogx + a/(flogx*flogx));
       l =   599 a = 0.30
       l =  1500 a = 0.80
       l =  2700 a = 0.90
       l =  3300 a = 1.00
       l =  3500 a = 1.10
       l =  5400 a = 1.20
       l =  5500 a = 1.30
       l = 11800 a = 1.40
       l = 11900 a = 1.50
       l = 19400 a = 1.60
       l = 19500 a = 1.70
       l = 32299 a = 1.80
       l = 32400 a = 1.90
       l = 71000 a = 2.00
       l =176000 a = 2.10
       l =315000 a = 2.20
     */

  if (x >= W_CONST(10000000)) { /* Dusart's bound */
    bound = (fx/flogx) * (F1 + F1/flogx + 1.80/(flogx*flogx));
  } else if (x >= W_CONST( 315000)) {
    bound = (fx/flogx) * (F1 + F1/flogx + 2.20/(flogx*flogx));
  } else if (x >= W_CONST( 176000)) {
    bound = (fx/flogx) * (F1 + F1/flogx + 2.10/(flogx*flogx));
  } else if (x >= W_CONST( 32299)) {
    bound = (fx/flogx) * (F1 + F1/flogx + 1.80/(flogx*flogx));
  } else if (x >= W_CONST( 19400)) {
    bound = (fx/flogx) * (F1 + F1/flogx + 1.60/(flogx*flogx));
  } else if (x >= W_CONST(  5500)) {
    bound = (fx/flogx) * (F1 + F1/flogx + 1.30/(flogx*flogx));
  } else if (x >= W_CONST(  2700)) {
    bound = (fx/flogx) * (F1 + F1/flogx + 0.90/(flogx*flogx));
  } else if (x >= W_CONST(   599)) {
    bound = (fx/flogx) * (F1 + F1/flogx + 0.30/(flogx*flogx));
  } else {
    bound = fx / (flogx-0.7);
  }
  return (int) ( bound );
}
int prime_count_upper(WTYPE x)
{
  float fx;
  float flogx;
  float bound;

  if (x < NPRIME_COUNT_SMALL)
    return prime_count_small[x];

  fx = (float)x;
  flogx = logf(x);

  if        (x >= W_CONST(355991)) {
    bound = (fx/flogx) * (F1 + F1/flogx + 2.51/(flogx*flogx));
  } else if (x >= W_CONST(350000)) {
    bound = (fx/flogx) * (F1 + F1/flogx + 2.54/(flogx*flogx));
  } else if (x >= W_CONST(59000)) {
    bound = (fx/flogx) * (F1 + F1/flogx + 2.52/(flogx*flogx));
  } else if (x >= W_CONST(24000)) {
    bound = (fx/flogx) * (F1 + F1/flogx + 2.48/(flogx*flogx));
  } else if (x >= W_CONST(16000)) {
    bound = (fx/flogx) * (F1 + F1/flogx + 2.30/(flogx*flogx));
  } else {
    bound = fx / (flogx-1.098);
  }
  return (int) ( bound + F1 );
}
int prime_count(WTYPE x)
{
  WTYPE curprime;
  int count;

  if (x < NPRIME_COUNT_SMALL)
    return prime_count_small[x];

  /* Beware: astonishingly slow for large x */
  if      (x >= W_CONST(4476044)) { count =314368; curprime = 4476044; }
  else if (x >= W_CONST(1001250)) { count = 78592; curprime = 1001250; }
  else if (x >= W_CONST( 220420)) { count = 19648; curprime =  220420; }
  else if (x >= W_CONST(  47658)) { count =  4912; curprime =   47658; }
  else if (x >= W_CONST(  10000)) { count =  1228; curprime =   10000; }
  else                            { count =    18; curprime =      67; }
  while (curprime <= x) {
    curprime = next_prime(curprime);
    count++;
  }
  return count;
}

WTYPE* sieve_base(WTYPE end)
{
  WTYPE start = 0;
  WTYPE* mem;
  size_t sieve_val, s;
  size_t last = (end+1)/2;

  mem = (WTYPE*) calloc( NWORDS(last), sizeof(WTYPE) );
  if (mem == 0) {
    croak("allocation failure in sieve_base: could not alloc %lu bits", end+1);
    return 0;
  }
  for (sieve_val = 3; (sieve_val*sieve_val) <= end; sieve_val += 2) {
    if (!IS_SET_ARRAY_BIT(mem,sieve_val/2)) {
      for (s = sieve_val*sieve_val; s <= end; s += 2*sieve_val) {
        SET_ARRAY_BIT(mem,s/2);
      }
    }
  }
  return mem;
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
    int e = p->maxlen;
    p->maxlen = 0;
    p->curlen = 0;
    croak("allocation failure in primearray: could not alloc %d entries", e);
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
  /* This should be good as long as the upper bound is reasonably tight */
  if ( ! expand_primearray_index(p, prime_count_upper(value)+1) )
    return 0;
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
