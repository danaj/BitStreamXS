#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <math.h>

/********************  functions to support sequences  ********************/

#include "sequences.h"

/********************  primes  ********************/

static WTYPE count_zero_bits(const unsigned char* m, WTYPE nbytes)
{
  WTYPE count = 0;
  assert( (nbytes % sizeof(WTYPE)) == 0 );
  assert( (((WTYPE)m) % sizeof(WTYPE)) == 0);
  {
    const WTYPE* wm = (const WTYPE*) m;
    WTYPE nwords = nbytes / sizeof(WTYPE);
    while (nwords-- > 0) {
      /* Count 0 bits using Wegner/Lehmer/Kernighan method. */
      WTYPE word = ~ *wm++;
      while (word) {
        word &= word-1;
        count++;
      }
    }
  }
  return count;
}


static unsigned char* prime_cache_sieve = 0;
static WTYPE  prime_cache_size = 0;

/* Get the maximum sieved value of the cached prime sieve. */
WTYPE _get_prime_cache_size(void)
{
  return prime_cache_size;
}

/*
 * Get the size and a pointer to the cached prime sieve.
 * Returns the maximum sieved value in the sieve.
 * Allocates and sieves if needed.
 *
 * The sieve holds 30 numbers per byte, using a mod-30 wheel.
 */
WTYPE get_prime_cache(WTYPE n, const unsigned char** sieve)
{
  n = ((n+1)/2)*2;  /* bump n up to next even number */

  if (prime_cache_size < n) {

    if (prime_cache_sieve != 0)
      free(prime_cache_sieve);
    prime_cache_size = 0;

    /* Sieve a bit more than asked, to mitigate thrashing */
    n += 1000;

    prime_cache_sieve = sieve_erat30(n);

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

int is_prime(WTYPE n)
{
  WTYPE d, m;
  unsigned char mtab;

  if (n <= 31)
    return ((SMALL_PRIMES_MASK >> n) & 1);

  d = n/30;
  m = n - d*30;
  mtab = masktab30[m];  /* Bitmask in mod30 wheel */

  /* Return 0 if not a multiple of 2, 3, or 5 */
  if (mtab == 0)
    return 0;

  if (n <= prime_cache_size)
    return ((prime_cache_sieve[d] & mtab) == 0);

  /* Trial division, mod 30 */
  return _is_prime7(n);
}

static const WTYPE prime_next_small[] =
  {2,2,3,5,5,7,7,11,11,11,11,13,13,17,17,17,17,19,19,23,23,23,23,
   29,29,29,29,29,29,31,31,37,37,37,37,37,37,41,41,41,41,43,43,47,
   47,47,47,53,53,53,53,53,53,59,59,59,59,59,59,61,61,67,67,67,67,67,67,71};
#define NPRIME_NEXT_SMALL (sizeof(prime_next_small)/sizeof(prime_next_small[0]))

WTYPE next_trial_prime(WTYPE n)
{
  static const WTYPE L = 30;
  static const WTYPE indices[] = {1, 7, 11, 13, 17, 19, 23, 29};
  static const WTYPE M = 8;
  WTYPE d;
  int index;

  if (n < NPRIME_NEXT_SMALL)
    return prime_next_small[n];

  n++;
  d = n/L;
  index = 0;   while ((n-d*L) > indices[index])  index++;
  n = L*d + indices[index];
  while (!_is_prime7(n)) {
    if (++index == M) { d++; index = 0; }
    n = L*d + indices[index];
  }
  return n;
}

WTYPE next_prime(WTYPE n)
{
  WTYPE d, m;

  if (n < NPRIME_NEXT_SMALL)
    return prime_next_small[n];

  if (n < prime_cache_size) {
    START_DO_FOR_EACH_SIEVE_PRIME(prime_cache_sieve, n+1, prime_cache_size)
      return p;
    END_DO_FOR_EACH_SIEVE_PRIME;
    /* Not found, so must be larger than the cache size */
    n = prime_cache_size;
  }

  n++;
  d = n/30;
  m = n - d*30;
  m += distancewheel30[m];  /* m is on a wheel location */
  assert(masktab30[m] != 0);
  n = d*30+m;
  while (!_is_prime7(n)) {
    m = nextwheel30[m];  if (m == 1) d++;
    n = d*30+m;
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

static const UV prime_count_small[] =
  {0,0,1,2,2,3,3,4,4,4,4,5,5,6,6,6,6,7,7,8,8,8,8,9,9,9,9,9,9,10,10,
   11,11,11,11,11,11,12,12,12,12,13,13,14,14,14,14,15,15,15,15,15,15,
   16,16,16,16,16,16,17,17,18,18,18,18,18,18,19};
#define NPRIME_COUNT_SMALL  (sizeof(prime_count_small)/sizeof(prime_count_small[0]))

static const float F1 = 1.0;
UV prime_count_lower(WTYPE x)
{
  float fx, flogx;
  float a = 1.80;

  if (x < NPRIME_COUNT_SMALL)
    return prime_count_small[x];

  fx = (float)x;
  flogx = logf(x);

  if (x < 599)
    return (UV) (fx / (flogx-0.7));

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
  else if (x <W_CONST(0xFFFFFFFF)) {  a = 2.32; }

  return (UV) ( (fx/flogx) * (F1 + F1/flogx + a/(flogx*flogx)) );
}

UV prime_count_upper(WTYPE x)
{
  float fx, flogx;
  float a = 2.51;

  if (x < NPRIME_COUNT_SMALL)
    return prime_count_small[x];

  fx = (float)x;
  flogx = logf(x);

  /* This function is unduly complicated. */

  if (x < 1621)  return (UV) (fx / (flogx-1.048) + F1);
  if (x < 5000)  return (UV) (fx / (flogx-1.071) + F1);
  if (x < 15900) return (UV) (fx / (flogx-1.098) + F1);

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
  else if (x <W_CONST(0xFFFFFFFF)) {  a = 2.362; }

  /*
   * An alternate idea:
   *  float alog[23] = {  2.30,2.30,2.30,2.30,2.30,2.30,2.30 ,2.30,2.30,2.30,
   *                      2.47,2.49,2.53,2.50,2.49,2.49,2.456,2.44,2.40,2.370,
   *                      2.362,2.362,2.362,2.362};
   *  float clog[23] = {  0,   0,   0,   0,   0,   0,   0,    0,   0,   1,
   *                      3,   1,   2,   1,   3,   2,   5,   -6,   1,   1,
   *                      1,   1,   1,   1};
   *  if ((int)flogx < 23) {
   *    a = alog[(int)flogx];
   *    return ((UV) ( (fx/flogx) * (F1 + F1/flogx + a/(flogx*flogx)) ) + clog[(int)flogx] + 0.01);
   *  }
   *
   * Another thought is to use another term to help control the growth.
   */

  return (UV) ( (fx/flogx) * (F1 + F1/flogx + a/(flogx*flogx)) + F1 );
}

UV prime_count_approx(WTYPE x)
{
  /* Placeholder for fancy algorithms, like Tomás Oliveira e Silva's:
   *     http://trac.sagemath.org/sage_trac/ticket/8135
   * or an approximation to Li(x) plus a delta.
   */
  UV lower = prime_count_lower(x);
  UV upper = prime_count_upper(x);
  return ((lower + upper) / 2);
}

static const WTYPE primes_small[] =
  {0,2,3,5,7,11,13,17,19,23,29,31,37,41,43,47,53,59,61,67,71};
#define NPRIMES_SMALL (sizeof(primes_small)/sizeof(primes_small[0]))

/* The nth prime will be less than this number */
static UV nth_prime_upper(WTYPE n)
{
  float fn = (float) n;
  if (n < NPRIMES_SMALL)
    return primes_small[n]+1;
  return (UV) (fn * logf(fn)  +  fn * logf(logf(fn)) + 1.0);
}
/* The nth prime will be more than this number */
static UV nth_prime_lower(WTYPE n)
{
  float fn = (float) n;
  if (n < NPRIMES_SMALL)
    return (n==0) ? 0 : primes_small[n]-1;
  return (UV) (fn * logf(fn)  +  fn * logf(logf(fn)) - fn);
}

UV nth_prime(WTYPE n)
{
  const unsigned char* sieve;
  UV upper_limit, start, count, s, nwords_left;

  if (n < NPRIMES_SMALL)
    return primes_small[n];

  upper_limit = nth_prime_upper(n);
  /* The nth prime is guaranteed to be within this range */
  if (get_prime_cache(upper_limit, &sieve) < upper_limit) {
    croak("Couldn't generate sieve for nth(%lu) [sieve to %lu]", (unsigned long)n, (unsigned long)upper_limit);
    return 0;
  }

  count = 3;
  start = 7;
  s = 0;
  nwords_left = (n-count) / (8*sizeof(WTYPE));
  while ( nwords_left > 0 ) {
    /* There is at minimum one word we can count (and probably many more) */
    count += count_zero_bits(sieve+s*sizeof(WTYPE), nwords_left*sizeof(WTYPE));
    assert(count <= n);
    s += nwords_left;
    nwords_left = (n-count) / (8*sizeof(WTYPE));
  }
  if (s > 0)
    start = s * sizeof(WTYPE) * 30;

  START_DO_FOR_EACH_SIEVE_PRIME(sieve, start, upper_limit)
    if (++count == n)  return p;
  END_DO_FOR_EACH_SIEVE_PRIME;
  croak("nth_prime failed for %lu, not found in range %lu - %lu", (unsigned long)n, (unsigned long) start, (unsigned long)upper_limit);
  return 0;
}



void prime_init(WTYPE n)
{
  if ( (n == 0) && (prime_cache_sieve == 0) ) {
    /* On init, make a few primes */
    size_t initial_primes_to = 30 * (1024-8);
    prime_cache_sieve = sieve_erat30(initial_primes_to);
    if (prime_cache_sieve != 0)
      prime_cache_size = initial_primes_to;
    return;
  }

  get_prime_cache(n, 0);   /* Sieve to n */
}


UV prime_count(WTYPE n)
{
  const unsigned char* sieve;
  static WTYPE last_fw    = 0;
  static UV    last_count = 3;
  WTYPE s, full_words;
  UV count = 3;

  if (n < NPRIME_COUNT_SMALL)
    return prime_count_small[n];

  /* Get the cached sieve. */
  if (get_prime_cache(n, &sieve) < n) {
    croak("Couldn't generate sieve for prime_count");
    return 0;
  }

#if 0
  /* The really simple way -- walk the sieve */
  START_DO_FOR_EACH_SIEVE_PRIME(sieve, 7, n)
    count++;
  END_DO_FOR_EACH_SIEVE_PRIME;
#else
  full_words = n / (30*sizeof(WTYPE));
  s = 0;

  /* Start from last word position if we can.  This is a big speedup when
   * calling prime_count many times with successively larger numbers. */
  if (full_words >= last_fw) {
    s = last_fw;
    count = last_count;
  }

  count += count_zero_bits(sieve+s*sizeof(WTYPE), (full_words-s)*sizeof(WTYPE));

  last_fw    = full_words;
  last_count = count;

  START_DO_FOR_EACH_SIEVE_PRIME(sieve, 30*sizeof(WTYPE)*full_words, n)
    count++;
  END_DO_FOR_EACH_SIEVE_PRIME;
#endif

  return count;
}


WTYPE* sieve_erat(WTYPE end)
{
  WTYPE* mem;
  size_t n, s;
  size_t last = (end+1)/2;

  mem = (WTYPE*) calloc( NWORDS(last), sizeof(WTYPE) );
  if (mem == 0) {
    croak("allocation failure in sieve_erat: could not alloc %lu bits", (unsigned long)last);
    return 0;
  }

  n = 3;
  while ( (n*n) <= end) {
    for (s = n*n; s <= end; s += 2*n)
      SET_ARRAY_BIT(mem,s/2);
    do { n += 2; } while (IS_SET_ARRAY_BIT(mem,n/2));
  }

  SET_ARRAY_BIT(mem, 1/2);  /* 1 is composite */

  return mem;
}


/* Wheel 30 sieve.  Ideas from Terje Mathisen and Quesada / Van Pelt. */
unsigned char* sieve_erat30(WTYPE end)
{
  unsigned char* mem;
  size_t max_buf, buffer_words;
  WTYPE prime;

  max_buf = (end + 29) / 30;
  buffer_words = (end + (30*sizeof(WTYPE)) - 1) / (30*sizeof(WTYPE));
  mem = (unsigned char*) calloc( buffer_words, sizeof(WTYPE) );
  if (mem == 0) {
    croak("allocation failure in sieve_erat30: could not alloc %lu bytes", (unsigned long)(buffer_words*sizeof(WTYPE)));
    return 0;
  }

  /* Shortcut to mark 7.  Just an optimization. */
  if ( (7*7) <= end ) {
    WTYPE d = 1;
    while ( (d+6) < max_buf) {
      mem[d+0] = 0x20;  mem[d+1] = 0x10;  mem[d+2] = 0x81;  mem[d+3] = 0x08;
      mem[d+4] = 0x04;  mem[d+5] = 0x40;  mem[d+6] = 0x02;  d += 7;
    }
    if ( d < max_buf )  mem[d++] = 0x20;
    if ( d < max_buf )  mem[d++] = 0x10;
    if ( d < max_buf )  mem[d++] = 0x81;
    if ( d < max_buf )  mem[d++] = 0x08;
    if ( d < max_buf )  mem[d++] = 0x04;
    if ( d < max_buf )  mem[d++] = 0x40;
    assert(d >= max_buf);
  }
  for (prime = 11; (prime*prime) <= end; prime = next_prime_in_sieve(mem,prime)) {
    WTYPE d = (prime*prime)/30;
    WTYPE m = (prime*prime) - d*30;
    WTYPE dinc = (2*prime)/30;
    WTYPE minc = (2*prime) - dinc*30;
    WTYPE wdinc[8];
    unsigned char wmask[8];
    int i;

    /* Find the positions of the next composites we will mark */
    for (i = 1; i <= 8; i++) {
      WTYPE dlast = d;
      do {
        d += dinc;
        m += minc;
        if (m >= 30) { d++; m -= 30; }
      } while ( masktab30[m] == 0 );
      wdinc[i-1] = d - dlast;
      wmask[i%8] = masktab30[m];
    }
    d -= prime;
#if 0
    assert(d == ((prime*prime)/30));
    assert(d < max_buf);
    assert(prime = (wdinc[0]+wdinc[1]+wdinc[2]+wdinc[3]+wdinc[4]+wdinc[5]+wdinc[6]+wdinc[7]));
#endif
    i = 0;        /* Mark the composites */
    do {
      mem[d] |= wmask[i];
      d += wdinc[i];
      i = (i+1) & 7;
    } while (d < max_buf);
  }

  mem[0] |= masktab30[1];  /* 1 is composite */

  return mem;
}



int sieve_segment(unsigned char* mem, WTYPE startd, WTYPE endd)
{
  const unsigned char* sieve;
  WTYPE limit;
  WTYPE pcsize;
  WTYPE startp = 30*startd;
  WTYPE endp = 30*endd+29;
  WTYPE ranged = endd - startd + 1;

  assert(mem != 0);
  assert(endd >= startd);
  memset(mem, 0, ranged);

  limit = sqrt( (double) endp ) + 1;
  /* printf("segment sieve from %lu to %lu (aux sieve to %lu)\n", startp, endp, limit); */
  pcsize = get_prime_cache(limit, &sieve);
  if (pcsize < limit) {
    croak("Couldn't generate small sieve for segment sieve");
    return 0;
  }

  START_DO_FOR_EACH_SIEVE_PRIME(sieve, 7, pcsize)
  {
    /* p increments from 7 to at least sqrt(endp) */
    WTYPE p2 = p*p;
    if (p2 >= endp)  break;
    /* Find first multiple of p greater than p*p and larger than startp */
    if (p2 < startp) {
      p2 = (startp / p) * p;
      if (p2 < startp)  p2 += p;
    }
    /* Bump to next multiple that isn't divisible by 2, 3, or 5 */
    while (masktab30[p2%30] == 0) { p2 += p; }
    if (p2 < endp)  {
      /* Sieve from startd to endd starting at p2, stepping p */
#if 0
      /* Basic sieve */
      do {
        mem[(p2 - startp)/30] |= masktab30[p2%30];
        do { p2 += 2*p; } while (masktab30[p2%30] == 0);
      } while (p2 < endp);
#else
      WTYPE d = (p2)/30;
      WTYPE m = (p2) - d*30;
      WTYPE dinc = (2*p)/30;
      WTYPE minc = (2*p) - dinc*30;
      WTYPE wdinc[8];
      unsigned char wmask[8];
      int i;

      /* Find the positions of the next composites we will mark */
      for (i = 1; i <= 8; i++) {
        WTYPE dlast = d;
        do {
          d += dinc;
          m += minc;
          if (m >= 30) { d++; m -= 30; }
        } while ( masktab30[m] == 0 );
        wdinc[i-1] = d - dlast;
        wmask[i%8] = masktab30[m];
      }
      d -= p;
      i = 0;        /* Mark the composites */
      do {
        mem[d-startd] |= wmask[i];
        d += wdinc[i];
        i = (i+1) & 7;
      } while (d <= endd);
#endif
    }
  }
  END_DO_FOR_EACH_SIEVE_PRIME;

  return 1;
}



/********************  best pair (for Additive) ********************/

static int gamma_length(WTYPE n)
{
  int log2 = 0;
  while (n >= ((2 << log2)-1))  log2++;
  return ((2*log2)+1);
}

/* adder is used to modify the stored indices.  A function would be better. */
int find_best_pair(WTYPE* basis, int basislen, WTYPE val, int adder, int* a, int* b)
{
  int maxbasis;
  int bestlen = INT_MAX;
  int i, j;

  assert( (basis != 0) && (a != 0) && (b != 0) && (basislen >= 1) );
  /* Find how far in basis to look */
  if ((basislen > 15) && (val > basis[15])) {
    /* Binary search for large values */
    i = 0;
    j = basislen-1;
    while (i < j) {
      int mid = (i+j)/2;
      if (basis[mid] < val)   i = mid+1;
      else                    j = mid;
    }
    maxbasis = i-1;
  } else {
    /* Iteration for small values */
    maxbasis = 0;
    while ( ((maxbasis+1) < basislen) && (basis[maxbasis+1] < val) )
      maxbasis++;
  }
  assert(maxbasis < basislen);
  assert(basis[maxbasis] <= val);
  assert( ((maxbasis+1) == basislen) || (basis[maxbasis+1] >= val) );

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

int find_best_prime_pair(WTYPE val, int adder, int* a, int* b)
{
  int bestlen = INT_MAX;
  int i, j;
  WTYPE pi, pj;
  const unsigned char* sieve;

  assert( (a != 0) && (b != 0) );

  if (get_prime_cache(val, &sieve) < val) {
    croak("Couldn't generate sieve for find_best_prime_pair");
    return 0;
  }

  pi = 1;
  pj = prev_prime_in_sieve(sieve,val+1);
  i = 0;
  j = (val <= 2) ? 1 : prime_count(pj)-1;
  while (i <= j) {
    WTYPE sum = pi + pj;
    if (sum > val) {
      j--;
      pj = (j == 0) ? 1 : prev_prime_in_sieve(sieve,pj);
    } else {
      if (sum == val) {
        int p1 = i + adder;
        int p2 = j - i + adder;
        int glen = gamma_length(p1) + gamma_length(p2);
        if (glen < bestlen) {
          *a = p1;
          *b = p2;
          bestlen = glen;
        }
      }
      i++;
      pi = (i == 1) ? 3 : next_prime_in_sieve(sieve,pi);
    }
  }
  return (bestlen < INT_MAX);
}
