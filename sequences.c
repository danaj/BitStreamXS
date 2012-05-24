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
 *    pi_upper(x) - pi(x)                      pi_lower(x) - pi(x)
 *    <    10   for x <         5_371          <    10   for x <        9_437
 *    <    50   for x <       295_816          <    50   for x <      136_993
 *    <   100   for x <     1_761_655          <   100   for x <      909_911
 *    <   200   for x <     9_987_821          <   200   for x <    8_787_901
 *    <   400   for x <    34_762_891          <   400   for x <   30_332_723
 *    <  1000   for x <   372_748_528          <  1000   for x <  233_000_533
 *    <  5000   for x < 1_882_595_905          <  5000   for x <  over 4300M
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
 * I have tweaked the bounds formulas for small (under 2000M) numbers so they
 * are tighter.  These bounds are verified via trial.  The Dusart bounds
 * (1.8 and 2.51) are used for larger numbers since those are proven.
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
  float fx, flogx;
  float a = 1.80;

  if (x < NPRIME_COUNT_SMALL)
    return prime_count_small[x];

  fx = (float)x;
  flogx = logf(x);

  if (x < 599)
    return (int) (fx / (flogx-0.7));

  if      (x <     2700) {  a = 0.30; }
  else if (x <     5500) {  a = 0.90; }
  else if (x <    19400) {  a = 1.30; }
  else if (x <    32299) {  a = 1.60; }
  else if (x <   176000) {  a = 1.80; }
  else if (x <   315000) {  a = 2.10; }
  else if (x <  1100000) {  a = 2.20; }
  else if (x <  4500000) {  a = 2.31; }
  else if (x <233000000) {  a = 2.36; }
  else if (x <240000000) {  a = 2.32; }
  //else if (x <W_CONST(40000000000)) {  a = 2.35; }
  else { a = 2.32; }

  return (int) ( (fx/flogx) * (F1 + F1/flogx + a/(flogx*flogx)) );
}
int prime_count_upper(WTYPE x)
{
  float fx, flogx;
  float a = 2.51;

  if (x < NPRIME_COUNT_SMALL)
    return prime_count_small[x];

  fx = (float)x;
  flogx = logf(x);

  if (x < 16000)
    return (int) (fx / (flogx-1.098) + F1);

  if      (x <    24000) {  a = 2.30; }
  else if (x <    59000) {  a = 2.48; }
  else if (x <   350000) {  a = 2.52; }
  else if (x <   355991) {  a = 2.54; }
  else if (x <   356000) {  a = 2.51; }
  else if (x <  3550000) {  a = 2.50; }
  else if (x <  3560000) {  a = 2.49; }
  else if (x <  5000000) {  a = 2.48; }
  else if (x <  8000000) {  a = 2.47; }
  else if (x < 13000000) {  a = 2.46; }
  else if (x < 18000000) {  a = 2.45; }
  else if (x < 31000000) {  a = 2.44; }
  else if (x < 41000000) {  a = 2.43; }
  else if (x < 48000000) {  a = 2.42; }
  else if (x <119000000) {  a = 2.41; }
  else if (x <182000000) {  a = 2.40; }
  else if (x <192000000) {  a = 2.395; }
  else if (x <213000000) {  a = 2.390; }
  else if (x <271000000) {  a = 2.385; }
  else if (x <322000000) {  a = 2.380; }
  else if (x <400000000) {  a = 2.375; }
  else if (x <510000000) {  a = 2.370; }
  else if (x <682000000) {  a = 2.367; }
  //else if (x <W_CONST(40000000000)) {  a = 2.49; }
  else { a = 2.362; }

  return (int) ( (fx/flogx) * (F1 + F1/flogx + a/(flogx*flogx)) + F1 );
}
int prime_count(WTYPE n)
{
  WTYPE curprime;
  int count;

  if (n < NPRIME_COUNT_SMALL)
    return prime_count_small[n];

  /* For suffiently large n, we should sieve. */
  if (n > W_CONST(100000)) {
    WTYPE s;
    WTYPE* sieve = sieve_base23(n);
    count = 2;
    n = (n-1)/2;
    s = W_CONST(5);
    while (s <= n) {
      if (!IS_SET_ARRAY_BIT(sieve, s))
        count++;
      s++;
      if ( (s <= n) && (!IS_SET_ARRAY_BIT(sieve, s)) )
        count++;
      s += 2;
    }
    free(sieve);
  } else {
    /* Trial division */
    if      (n >= W_CONST(4476044)) { count =314368; curprime = 4476044; }
    else if (n >= W_CONST(1001250)) { count = 78592; curprime = 1001250; }
    else if (n >= W_CONST( 220420)) { count = 19648; curprime =  220420; }
    else if (n >= W_CONST(  47658)) { count =  4912; curprime =   47658; }
    else if (n >= W_CONST(  10000)) { count =  1228; curprime =   10000; }
    else                            { count =    18; curprime =      67; }
    while (curprime <= n) {
      curprime = next_prime(curprime);
      count++;
    }
  }
  return count;
}

WTYPE* sieve_base(WTYPE end)
{
  WTYPE start = 0;
  WTYPE* mem;
  size_t n, s;
  size_t last = (end+1)/2;

  mem = (WTYPE*) calloc( NWORDS(last), sizeof(WTYPE) );
  if (mem == 0) {
    croak("allocation failure in sieve_base: could not alloc %lu bits", end+1);
    return 0;
  }
  /* We could mask words to do quick small prime marking word at a time */
  for (n = 3; (n*n) <= end; n += 2) {
    if (!IS_SET_ARRAY_BIT(mem,n/2)) {
      for (s = n*n; s <= end; s += 2*n) {
        SET_ARRAY_BIT(mem,s/2);
      }
    }
  }
  return mem;
}

/* Skip 2 and 3 */
WTYPE* sieve_base23(WTYPE end)
{
  WTYPE start = 0;
  WTYPE* mem;
  size_t n, s;
  size_t last = (end+1)/2;

  mem = (WTYPE*) calloc( NWORDS(last), sizeof(WTYPE) );
  if (mem == 0) {
    croak("allocation failure in sieve_base: could not alloc %lu bits", end+1);
    return 0;
  }
  n = 5;
  while ( (n*n) <= end ) {
    if (!IS_SET_ARRAY_BIT(mem,n/2)) {
      for (s = n*n; s <= end; s += 2*n) {
        SET_ARRAY_BIT(mem,s/2);
      }
    }
    n += 2;
    if ( ((n*n) <= end) && (!IS_SET_ARRAY_BIT(mem,n/2)) ) {
      for (s = n*n; s <= end; s += 2*n) {
        SET_ARRAY_BIT(mem,s/2);
      }
    }
    n += 4;
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
