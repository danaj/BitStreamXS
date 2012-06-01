#ifndef DBXS_SEQUENCES_H
#define DBXS_SEQUENCES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "wtype.h"

extern WTYPE _get_prime_cache_size(void);
extern WTYPE get_prime_cache(WTYPE n, const unsigned char** sieve);
extern int   is_prime(WTYPE x);
extern WTYPE next_trial_prime(WTYPE x);
extern WTYPE next_prime(WTYPE x);

extern WTYPE prime_count_lower(WTYPE x);
extern WTYPE prime_count_upper(WTYPE x);
extern WTYPE prime_count_approx(WTYPE x);
extern WTYPE prime_count(WTYPE x);

extern WTYPE nth_prime(WTYPE n);

extern void  prime_init(WTYPE x);
extern WTYPE* sieve_erat(WTYPE end);
extern unsigned char* sieve_erat30(WTYPE end);
extern int sieve_segment(unsigned char* mem, WTYPE startd, WTYPE endd);

extern int   find_best_pair(WTYPE* basis, int basislen, WTYPE val, int adder, int* a, int* b);
extern int   find_best_prime_pair(WTYPE val, int adder, int* a, int* b);



#define SET_ARRAY_BIT(ar,n) \
   ar[(n)/BITS_PER_WORD]  |=  (W_ONE << ((n)%BITS_PER_WORD))
#define XOR_ARRAY_BIT(ar,n) \
   ar[(n)/BITS_PER_WORD]  ^=  (W_ONE << ((n)%BITS_PER_WORD))
#define CLR_ARRAY_BIT(ar,n) \
   ar[(n)/BITS_PER_WORD]  &=  ~(W_ONE << ((n)%BITS_PER_WORD))
#define IS_SET_ARRAY_BIT(ar,n) \
   (ar[(n)/BITS_PER_WORD] & (W_ONE << ((n)%BITS_PER_WORD)) )

static const WTYPE wheel30[] = {1, 7, 11, 13, 17, 19, 23, 29};
/* Used for moving between primes */
static unsigned char nextwheel30[30] = {
    1,  7,  7,  7,  7,  7,  7, 11, 11, 11, 11, 13, 13, 17, 17,
   17, 17, 19, 19, 23, 23, 23, 23, 29, 29, 29, 29, 29, 29,  1 };
static unsigned char prevwheel30[30] = {
   29, 29,  1,  1,  1,  1,  1,  1,  7,  7,  7,  7, 11, 11, 13,
   13, 13, 13, 17, 17, 19, 19, 19, 19, 23, 23, 23, 23, 23, 23 };
/* The bit mask within a byte */
static unsigned char masktab30[30] = {
    0,  1,  0,  0,  0,  0,  0,  2,  0,  0,  0,  4,  0,  8,  0,
    0,  0, 16,  0, 32,  0,  0,  0, 64,  0,  0,  0,  0,  0,128  };
/* Add this to a number and you'll ensure you're on a wheel location */
static unsigned char distancewheel30[30] = {
    1,  0,  5,  4,  3,  2,  1,  0,  3,  2,  1,  0,  1,  0,  3,
    2,  1,  0,  1,  0,  3,  2,  1,  0,  5,  4,  3,  2,  1,  0 };

static int is_prime_in_sieve(const unsigned char* sieve, WTYPE p) {
  WTYPE d = p/30;
  WTYPE m = p - d*30;
  /* If m isn't part of the wheel, we return 0 */
  return ( (masktab30[m] != 0) && ((sieve[d] & masktab30[m]) == 0) );
}

/* Warning -- can go off the end of the sieve */
static WTYPE next_prime_in_sieve(const unsigned char* sieve, WTYPE p) {
  WTYPE d, m;
  if (p < 7)
    return (p < 2) ? 2 : (p < 3) ? 3 : (p < 5) ? 5 : 7;
  d = p/30;
  m = p - d*30;
  do {
    if (m==29) { d++; m = 1; while (sieve[d] == 0xFF) d++; }
    else       { m = nextwheel30[m]; }
  } while (sieve[d] & masktab30[m]);
  return(d*30+m);
}
static WTYPE prev_prime_in_sieve(const unsigned char* sieve, WTYPE p) {
  WTYPE d, m;
  if (p <= 7)
    return (p <= 2) ? 0 : (p <= 3) ? 2 : (p <= 5) ? 3 : 5;
  d = p/30;
  m = p - d*30;
  do {
    m = prevwheel30[m];  if (m==29) { if (d == 0) return 0;  d--; }
  } while (sieve[d] & masktab30[m]);
  return(d*30+m);
}

/* Useful macros for the wheel-30 sieve array */
#define START_DO_FOR_EACH_SIEVE_PRIME(sieve, a, b) \
  { \
    WTYPE p = a; \
    WTYPE l_ = b; \
    WTYPE d_ = p/30; \
    WTYPE m_ = p-d_*30; \
    m_ += distancewheel30[m_]; \
    p = d_*30 + m_; \
    while ( p <= l_ ) { \
      if ((sieve[d_] & masktab30[m_]) == 0)

#define END_DO_FOR_EACH_SIEVE_PRIME \
      m_ = nextwheel30[m_];  if (m_ == 1) { d_++; } \
      p = d_*30+m_; \
    } \
  }


#endif
