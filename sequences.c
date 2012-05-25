#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

/********************  functions to support sequences  ********************/

#include "sequences.h"

/********************  primes  ********************/

static WTYPE* prime_cache_sieve = 0;
static WTYPE  prime_cache_size  = 0;

/* Get the maximum sieved value of the cached prime sieve. */
WTYPE get_prime_cache_size(void)
{
  return prime_cache_size;
}

/*
 * Get the size and a pointer to the cached prime sieve.
 * Returns the maximum sieved value in the sieve.
 * The sieve is word based, where each bit b corresponds to n = 2*b+1.
 * Allocates and sieves if needed.
 */
WTYPE get_prime_cache(WTYPE n, const WTYPE** sieve)
{
  n = ((n+1)/2)*2;  /* bump n up to next even number */

  if (prime_cache_size < n) {

    if (prime_cache_sieve != 0)
      free(prime_cache_sieve);
    prime_cache_size = 0;

    /* Sieve a bit more than asked, to mitigate thrashing */
    n += 1000;

#if 1
    if (n > 100000)
      prime_cache_sieve = sieve_atkins(n);
    else
      prime_cache_sieve = sieve_erat(n);
#else
    prime_cache_sieve = sieve_erat(n);
#endif

    if (prime_cache_sieve != 0)
      prime_cache_size = n;
  }

  if (sieve != 0)
    *sieve = prime_cache_sieve;
  return prime_cache_size;
}

/* primes 2,3,5,7,11,13,17,19,23,29,31 as bits */
#define SMALL_PRIMES_MASK W_CONST(2693408940)
/* if (MOD235_MASK >> (n%30)) & 1, n is a multiple of 2, 3, or 5. */
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

int is_prime(WTYPE x)
{
  if (x <= 31)
    return ((SMALL_PRIMES_MASK >> x) & 1);

  WTYPE q, i;
  if ( (MOD235_MASK >> (x%30)) & 1 )
    return 0;

  return _is_prime7(x);
}

static const long prime_next_small[] = 
  {2,2,3,5,5,7,7,11,11,11,11,13,13,17,17,17,17,19,19,23,23,23,23,
   29,29,29,29,29,29,31,31,37,37,37,37,37,37,41,41,41,41,43,43,47,
   47,47,47,53,53,53,53,53,53,59,59,59,59,59,59,61,61,67,67,67,67,67,67,71};
#define NPRIME_NEXT_SMALL (sizeof(prime_next_small)/sizeof(prime_next_small[0]))
WTYPE next_prime(WTYPE x)
{
  static const WTYPE L = 30;
  WTYPE k0, n;
  static const WTYPE indices[] = {1, 7, 11, 13, 17, 19, 23, 29};
  static const WTYPE M = 8;
  int index;

  if (x < NPRIME_NEXT_SMALL)
    return prime_next_small[x];

  x++;
  k0 = x/L;
  index = 0;   while ((x-k0*L) > indices[index])  index++;
  n = L*k0 + indices[index];
  while (!_is_prime7(n)) {
    if (++index == M) {  k0++; index = 0; }
    n = L*k0 + indices[index];
  }
  return n;
}

/*
 * The pi(x) prime count functions.  prime_count(x) gives an exact number,
 * but requires determining all the primes up to x, so will be much slower.
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
 * The average of the upper and lower bounds is within 9 for all x < 15809, and
 * within 50 for all x < 1_763_367.
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
 * I have tweaked the bounds formulas for small (under 4000M) numbers so they
 * are tighter.  These bounds are verified via trial.  The Dusart bounds
 * (1.8 and 2.51) are used for larger numbers since those are proven.
 *
 */

static const long prime_count_small[] = 
  {0,0,1,2,2,3,3,4,4,4,4,5,5,6,6,6,6,7,7,8,8,8,8,9,9,9,9,9,9,10,10,
   11,11,11,11,11,11,12,12,12,12,13,13,14,14,14,14,15,15,15,15,15,15,
   16,16,16,16,16,16,17,17,18,18,18,18,18,18,19};
#define NPRIME_COUNT_SMALL  (sizeof(prime_count_small)/sizeof(prime_count_small[0]))
static const float F1 = 1.0;
long prime_count_lower(WTYPE x)
{
  float fx, flogx;
  float a = 1.80;

  if (x < NPRIME_COUNT_SMALL)
    return prime_count_small[x];

  fx = (float)x;
  flogx = logf(x);

  if (x < 599)
    return (long) (fx / (flogx-0.7));

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

  return (long) ( (fx/flogx) * (F1 + F1/flogx + a/(flogx*flogx)) );
}
long prime_count_upper(WTYPE x)
{
  float fx, flogx;
  float a = 2.51;

  if (x < NPRIME_COUNT_SMALL)
    return prime_count_small[x];

  fx = (float)x;
  flogx = logf(x);

  if (x < 16000)
    return (long) (fx / (flogx-1.098) + F1);

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

  return (long) ( (fx/flogx) * (F1 + F1/flogx + a/(flogx*flogx)) + F1 );
}

long prime_count(WTYPE n)
{
  const WTYPE* sieve;
  WTYPE s;
  long count, full_words;
  static WTYPE last_fw = 1;
  static long  last_count = 18;

  if (n < NPRIME_COUNT_SMALL)
    return prime_count_small[n];

  /* Get the cached sieve. */
  if (get_prime_cache(n, &sieve) < n) {
    croak("Couldn't generate sieve for prime_count");
    return 0;
  }
  n = (n-1)/2;
  full_words = NWORDS(n) - 1;

  /*   count = 2;
       for (s = W_CONST(5/2); s <= n; s++)
         if ( ! IS_SET_ARRAY_BIT(sieve, s) )
           count++;
       return count;      */

  count = 1;
  s = 0;

  /* Start from last word position if we can.  This is a big speedup when
     calling prime_count many times with successively larger numbers. */
  if (full_words >= last_fw) {
    s = last_fw;
    count = last_count;
  }

  /* Count 0 bits using Wegner/Lehmer/Kernighan method. */
  for (; s < full_words; s++) {
    WTYPE word = ~sieve[s];
    while (word) {
      word &= word-1;
      count++;
    }
  }

  last_fw    = full_words;
  last_count = count;

  /* Count primes in the last (partial) word */
  for (s = full_words*BITS_PER_WORD; s <= n; s++)
    if ( ! IS_SET_ARRAY_BIT(sieve, s) )
      count++;

  return count;
}


WTYPE* sieve_erat(WTYPE end)
{
  WTYPE* mem;
  size_t n, s;
  size_t last = (end+1)/2;

  mem = (WTYPE*) calloc( NWORDS(last), sizeof(WTYPE) );
  if (mem == 0) {
    croak("allocation failure in sieve_base: could not alloc %lu bits", (unsigned long)last);
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

  /* Mark 1 (2*0+1) as composite */
  SET_ARRAY_BIT(mem, 0);

  return mem;
}


WTYPE* sieve_atkins(WTYPE end)
{
  WTYPE* mem;
  size_t n, s, k;
  size_t last = (end+1+1)/2;
  long loopend, y_limit, dn;

  end++;
  /* One bit per odd number */
  mem = (WTYPE*) malloc( NWORDS(last) * sizeof(WTYPE) );
  if (mem == 0) {
    croak("allocation failure in sieve_atkins: could not alloc %lu bits", (unsigned long)last);
    return 0;
  }
  /* mark everything as a composite */
  memset(mem, 0xFF, NBYTES(last));

  {
    long xx3 = 3;
    long dxx;
    loopend = 12 * (long) sqrtf(((float)end-1.0)/3.0);
    for (dxx = 0; dxx < loopend; dxx += 24) {
      xx3 += dxx;
      y_limit = (long) (12.0*sqrtf( (float)end - (float)xx3 )) - 36;
      n = xx3 + 16;
      for (dn = -12; dn < (y_limit+1); dn += 72) {
        n += dn;
        XOR_ARRAY_BIT(mem,n/2);
      }
      n = xx3 + 4;
      for (dn = 12; dn < (y_limit+1); dn += 72) {
        n += dn;
        XOR_ARRAY_BIT(mem,n/2);
      }
    }
  }

  {
    long xx4 = 0;
    long dxx4;
    loopend = 4 + 8 * (long) sqrtf(((float)end-1.0)/4.0);
    for (dxx4 = 4; dxx4 < loopend; dxx4 += 8) {
      xx4 += dxx4;
      n = xx4 + 1;
      if (xx4%3) {
        y_limit = 4 * (long)sqrtf( (float)end - (float)xx4 ) - 3;
        for (dn = 0; dn < y_limit; dn += 8) {
          n += dn;
          XOR_ARRAY_BIT(mem,n/2);
        }
      } else {
        y_limit = 12 * (long)sqrtf( (float)end - (float)xx4 ) - 36;
        n = xx4 + 25;
        for (dn = -24; dn < (y_limit+1); dn += 72) {
          n += dn;
          XOR_ARRAY_BIT(mem,n/2);
        }
        n = xx4 + 1;
        for (dn = 24; dn < (y_limit+1); dn += 72) {
          n += dn;
          XOR_ARRAY_BIT(mem,n/2);
        }
      }
    }
  }

  {
    long xx = 1;
    long x;
    loopend = (long) sqrtf((float)end/2.0) + 1;
    for (x = 3; x < loopend; x += 2) {
      xx += 4*x - 4;
      n = 3*xx;
      if (n > end) {
        long min_y = (( (long) (sqrtf(n - end)) >>2)<<2);
        long yy = min_y * min_y;
        n -= yy;
        s = 4*min_y + 4;
      } else {
        s = 4;
      }
      for (dn = s; dn < 4*x; dn += 8) {
        n -= dn;
        if ((n <= end) && ((n%12) == 11))
          XOR_ARRAY_BIT(mem,n/2);
      }
    }

    xx = 0;
    loopend = (long) sqrtf((float)end/2.0) + 1;
    for (x = 2; x < loopend; x += 2) {
      xx += 4*x - 4;
      n = 3*xx;
      if (n > end) {
        long min_y = (( (long) (sqrtf(n - end)) >>2)<<2)-1;
        long yy = min_y * min_y;
        n -= yy;
        s = 4*min_y + 4;
      } else {
        n--;
        s = 0;
      }
      for (dn = s; dn < 4*x; dn += 8) {
        n -= dn;
        if ((n <= end) && ((n%12) == 11))
          XOR_ARRAY_BIT(mem,n/2);
      }
    }
  }

  /* Mark all squares of primes as composite */
  loopend = (long) sqrtf(end) + 1;
  for (n = 5; n < loopend; n += 2)
    if (!IS_SET_ARRAY_BIT(mem,n/2))
      for (k = n*n; k <= end; k += 2*n*n)
        SET_ARRAY_BIT(mem,k/2);

  /* Mark 3 as prime */
  CLR_ARRAY_BIT(mem, 3/2);

  return mem;
}




/********************  Goldbach primearray  ********************/

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
#if 1
  /* This should be good as long as the upper bound is reasonably tight */
  if ( ! expand_primearray_index(p, prime_count_upper(value)+1) )
    return 0;
#else
  const WTYPE* sieve;
  WTYPE low, high, s;
  int index;

  high = value + 2000;
  index = prime_count_upper(high);
  if (p->array == 0) {
    p->array = (WTYPE*) malloc((index+1) * sizeof(WTYPE));
    p->curlen = 0;
    p->maxlen = index+1;
  }
  if (p->maxlen <= index) {
    p->maxlen = index+1;
    p->array = (WTYPE*) realloc(p->array, p->maxlen * sizeof(WTYPE));
  }
  if (p->curlen <= 1) {
    p->array[0] = 2;
    p->array[1] = 3;
    p->curlen = 2;
  }
  /* We have enough room for the primes -- now fill them in. */
  low = p->array[p->curlen-1]+2;
  if (low > high)
    return 1;
  if (get_prime_cache(high, &sieve) < high) {
    croak("Couldn't generate sieve for expand_primarray_value");
    return 0;
  }
  low  = low/2;
  high = (high-1)/2;
  for (s = low; s <= high; s++) {
    if ( ! IS_SET_ARRAY_BIT(sieve, s) ) {
      assert(p->curlen <= p->maxlen);
      p->array[p->curlen++] = 2*s+1;
    }
  }
#endif
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
