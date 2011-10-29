#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "bitlist.h"

static int verbose = 0;
#define BITS_PER_WORD (8 * sizeof(WTYPE))
#define MAXBIT        (BITS_PER_WORD-1)
#define NWORDS(bits)  ( ((bits)+BITS_PER_WORD-1) / BITS_PER_WORD )

#define BIT_STACK_SIZE 32

static void expand_list(BitList *list, int len)
{
  if ( len > list->maxlen )
    resize(list, 1.10 * (len+4096) );
}

static char binstr[BITS_PER_WORD+1];
static char* word_to_bin(WTYPE word)
{
  int i;
  for (i = 0; i < BITS_PER_WORD; i++) {
    WTYPE bit = (word >> (MAXBIT-i)) & 1;
    binstr[i] = (bit == 0) ? '0' : '1';
  }
  binstr[BITS_PER_WORD] = '\0';
  return binstr;
}


BitList *new(int bits)
{
  BitList *list = (BitList *) malloc(sizeof (BitList));
  if (list == 0)
    return list;

  list->data = 0;
  list->pos = 0;
  list->len = 0;
  list->maxlen = 0;

  if (bits > 0)
    (void) resize(list, bits);

  return list;
}

int resize(BitList *list, int bits)
{
  if (bits == 0) {
    // erase everything
    if (list->data != 0) {
      free(list->data);
      list->data = 0;
    }
  } else {
    // Grow or shrink
    int oldwords = NWORDS(list->maxlen);
    int newwords = NWORDS(bits);
    list->data = (WTYPE*) realloc(list->data, newwords * sizeof(WTYPE));
    if ( (list->data != 0) && (newwords > oldwords) ) {
      // Zero out any new allocated space
      memset( list->data + oldwords,  0,  (newwords-oldwords)*sizeof(WTYPE) );
    }
    list->maxlen = bits;
  }
  if (list->data == 0) {
    list->maxlen = 0;
    list->len = 0;
    list->pos = 0;
  }
  return list->maxlen;
}

void DESTROY(BitList *list)
{
  if (list->data != 0)
    free(list->data);
  free(list);
}

void dump(BitList *list)
{
  int words = NWORDS(list->len);
  int i;
  for (i = 0; i < words; i++) {
    fprintf(stderr, "%2d %08x\n", i, list->data[i]);
  }
}

int getlen(BitList *list)        { return list->len; }
int getmaxlen(BitList *list)     { return list->maxlen; }
int getpos(BitList *list)        { return list->pos; }

int setlen(BitList *list, int newlen)
{
  // if less than maxlen...
  list->len = newlen;
  return list->len;
}
int setpos(BitList *list, int newpos)
{
  // if less than len...
  list->pos = newpos;
  return list->pos;
}

WTYPE vread(BitList *list, int bits)
{
  assert(bits > 0);
  assert(bits <= BITS_PER_WORD);

  WTYPE v = vreadahead(list, bits);
  list->pos += bits;
  return v;
}

WTYPE vreadahead(BitList *list, int bits)
{
  assert(bits > 0);
  assert(bits <= BITS_PER_WORD);

  WTYPE v = 0;
  int pos = list->pos;

  // Readahead can read past the end of the data, and requires that we fill
  // in with zeros.  We could force the data to always have BITS_PER_WORD of
  // empty space, or we can shift things here.
  int shift = (pos+bits) - list->len;

  if (shift > 0) {
    bits -= shift;
  }

  int wpos = pos / BITS_PER_WORD;
  int bpos = pos % BITS_PER_WORD;
  int wlen = BITS_PER_WORD - bits;

  if (bpos <= wlen) {
    // Single word read
    v = (list->data[wpos] >> (wlen-bpos)) & (~0UL >> wlen);
  } else {
    // Double word read
    int bits1 = BITS_PER_WORD - bpos;
    int bits2 = bits - bits1;
    v =   ( (list->data[wpos+0] & (~0UL >> bpos)) << bits2 )
        | ( list->data[wpos+1] >> (BITS_PER_WORD - bits2) );
  }

  // Don't change the actual position
  // pos += bits;

  if (shift > 0) {
    v <<= shift;
  }

  return v;
}

void vwrite(BitList *list, int bits, WTYPE value)
{
  int len = list->len;

  // expand the data if necessary
  expand_list(list, len+bits);

  if (value == 0) {
    list->len += bits;
    return;
  }

  assert(bits > 0);
  assert(bits <= BITS_PER_WORD);

  // mask value if needed
  if (bits < BITS_PER_WORD) {
    value &= (~0UL >> (BITS_PER_WORD - bits));
    assert(value < (1UL << bits));
  }

  //fprintf(stderr, "writing %d bits at len %d (%d)\n", bits, len, value);
#if 0
  // Simple write
  while (bits > 0) {
    int wpos = len / BITS_PER_WORD;
    int bpos = len % BITS_PER_WORD;
    WTYPE* wptr = list->data + wpos;
    WTYPE bit = (value >> (bits-1)) & 1;
    *wptr |= (bit << ( MAXBIT - bpos));
    //fprintf(stderr, "w%3d/%2d=%d [%s]\n",
    //        wpos, (BITS_PER_WORD-1) - bpos, bit, word_to_bin(*wptr));
    bits--;
    len++;
  }
  list->len = len;
  return;
#endif

  if (value == 1) {   // Optimize value 1 for any bits
    len += bits-1;
    bits = 1;
  }

  int wpos = len / BITS_PER_WORD;
  int bpos = len % BITS_PER_WORD;
  int wlen = BITS_PER_WORD - bits;

  if (bpos <= wlen) {
    // Single word write
    list->data[wpos] |= (value & (~0UL >> wlen)) << (wlen-bpos);
  } else {
    // Two word write
    int first_bits = BITS_PER_WORD - bpos;
    list->data[wpos++] |=  value >> (bits - first_bits);
    int wlen = BITS_PER_WORD - (bits - first_bits);
    list->data[wpos] |= (value & (~0UL >> wlen)) << (wlen-0);
  }

  list->len = len + bits;
}

void put_string(BitList *list, char* s)
{
  // Write words.  Reasonably fast.
  WTYPE word;
  int bits;
  while (*s != '\0') {
    word = 0;
    for (bits = 0;  (*s != 0) && (bits < BITS_PER_WORD);  bits++, s++) {
      word = (word << 1) | (*s != '0');
    }
    assert(bits > 0);
    vwrite(list, bits, word);
  }
}

char* read_string(BitList *list, int bits)
{
  int pos = list->pos;
  if ((pos + bits) > list->len)
    return 0;
  char* buf = (char*) malloc(bits+1);
  if (buf == 0)
    return buf;
#if 0
  int b;
  for (b = 0; b < bits; b++) {
    int wpos = pos / BITS_PER_WORD;
    int bpos = pos % BITS_PER_WORD;
    if ( ((list->data[wpos] << bpos) & (1UL << MAXBIT)) == 0 )
      buf[b] = '0';
    else
      buf[b] = '1';
    pos++;
  }
  list->pos = pos;
#endif
#if 1
  int wpos = pos / BITS_PER_WORD;
  int bpos = pos % BITS_PER_WORD;
  WTYPE word = list->data[wpos] << bpos;
  char*  bptr = buf;
  int b = bits;
  while (b-- > 0) {
    *bptr++ = ((word & (1UL << MAXBIT)) == 0) ? '0' : '1';
    word <<= 1;
    if (++bpos >= BITS_PER_WORD) {
      word = list->data[++wpos];
      bpos = 0;
      while (b >= BITS_PER_WORD) {
        if (word == 0UL) {
          memset(bptr, '0', BITS_PER_WORD);
          bptr += BITS_PER_WORD;
          b -= BITS_PER_WORD;
          word = list->data[++wpos];
        } else if (word == ~0UL) {
          memset(bptr, '1', BITS_PER_WORD);
          bptr += BITS_PER_WORD;
          b -= BITS_PER_WORD;
          word = list->data[++wpos];
        } else {
          break;
        }
      }
    }
  }
  list->pos = pos + bits;
#endif
  buf[bits] = '\0';
  return buf;
}

char* to_raw(BitList *list)
{
  int bits = list->len;
  int bytes = NWORDS(bits) * sizeof(WTYPE);
  char* buf = (char*) malloc(bytes);
  if (buf != 0) {
    list->pos = 0;
    char* bptr = buf;
    int b;
    for (b = 0; b < bytes; b++) {
      *bptr++ = vread(list, 8);
    }
  }
  return buf;
}
void from_raw(BitList *list, char* str, int bits)
{
  assert(str != 0);
  assert(bits >= 0);
  resize(list, bits);
  if (bits > 0) {
    int bytes = (bits + 7) / 8;
    list->pos = 0;
    list->len = 0;
    char* bptr = str;
    while (bytes-- > 0) {
      vwrite(list, 8, *bptr++);
    }
    list->len = bits;
  }
}



/*******************************************************************************
 *
 *                                      CODES
 *
 ******************************************************************************/

WTYPE get_unary (BitList *list)
{
  int pos = list->pos;

  // First word
  int wpos = pos / BITS_PER_WORD;
  int bpos = pos % BITS_PER_WORD;
  WTYPE *wptr = list->data + wpos;
  WTYPE word = (*wptr & (~0UL >> bpos)) << bpos;

  if (word == 0) {
    pos += (BITS_PER_WORD - bpos);
    while ( *++wptr == 0UL )
       pos += BITS_PER_WORD;
    word = *wptr;
  }
  assert(word != 0);

#if 0
  if ((word & 0xFFFFFFFF00000000UL) == 0UL) { pos += 32; word <<= 32; }
  if ((word & 0xFFFF000000000000UL) == 0UL) { pos += 16; word <<= 16; }
  if ((word & 0xFF00000000000000UL) == 0UL) { pos +=  8; word <<=  8; }
#endif
  while ( ((word >> MAXBIT) & 1UL) == 0UL ) {
    pos++;
    word <<= 1;
  }

  WTYPE v = pos - list->pos;
  list->pos = pos+1;
  return v;
}

void put_unary (BitList *list, WTYPE value)
{
#if 0
  vwrite(list, value+1, 1);
#else
  int len = list->len;
  int bits = value+1;

  expand_list(list, len+bits);

  len += value;

  int wpos = len / BITS_PER_WORD;
  int bpos = len % BITS_PER_WORD;

  list->data[wpos] |= (1UL << (MAXBIT - bpos));
  list->len = len + 1;
#endif
}

WTYPE get_unary1 (BitList *list)
{
  int pos = list->pos;
  WTYPE word;

  // First word
  int wpos = pos / BITS_PER_WORD;
  int bpos = pos % BITS_PER_WORD;
  if (bpos == 0)
    word = list->data[wpos];
  else
    word = (list->data[wpos] << bpos) | (~0UL >> (BITS_PER_WORD - bpos));

  if (word == ~0UL) {
    pos += (BITS_PER_WORD - bpos);
    while ( list->data[ pos / BITS_PER_WORD ] == ~0UL )
       pos += BITS_PER_WORD;
    word = list->data[ pos / BITS_PER_WORD ];
  }
  assert(word != ~0UL);

  while ( (word & (1UL << MAXBIT)) != 0UL ) {
    pos++;
    word <<= 1;
  }

  WTYPE v = pos - list->pos;
  list->pos = pos+1;
  return v;
}

void put_unary1 (BitList *list, WTYPE value)
{
#if 0
  // Simple code
  while (value > BITS_PER_WORD) {
    vwrite(list, BITS_PER_WORD, ~0UL);
    value -= BITS_PER_WORD;
  }
  if (value > 0)
    vwrite(list, value, ~0UL);
  vwrite(list, 1, 0UL);
#else
  int len = list->len;
  int bits = value+1;

  expand_list(list, len+value+1);

  int wpos = len / BITS_PER_WORD;
  int bpos = len % BITS_PER_WORD;
  int first_bits = BITS_PER_WORD - bpos;
  if ( (bpos > 0) && (first_bits <= value) ) {
    list->data[wpos++] |= (~0UL >> bpos);
    bpos = 0;
    value -= first_bits;
  }
  while (value > BITS_PER_WORD) {
    list->data[wpos++] = ~0UL;
    value -= BITS_PER_WORD;
  }
  if (value > 0)
    list->data[wpos] |= ( (~0UL << (BITS_PER_WORD-value)) >> bpos);

  list->len = len + bits;
#endif
}

WTYPE get_gamma (BitList *list)
{
  WTYPE base = get_unary(list);
  assert(base <= BITS_PER_WORD);
  WTYPE v;
  if (base == 0UL) {
    v = 0UL;
  } else if (base == BITS_PER_WORD) {
    v = ~0UL;
  } else {
    v = ( (1UL << base) | vread(list, base) ) - 1UL;
  }
  return v;
}

void put_gamma (BitList *list, WTYPE value)
{
  if (value == 0UL) {
    vwrite(list, 1, 1);
  } else if (value == ~0UL) {
    put_unary(list, BITS_PER_WORD);
  } else {
    WTYPE v = value+1;
    int base = 1;
    while ( (v >>= 1) != 0)
      base++;
    vwrite(list, base-1, 0UL);
    vwrite(list, base, value+1);
  }
}

WTYPE get_delta (BitList *list)
{
  WTYPE base = get_gamma(list);
  assert(base <= BITS_PER_WORD);
  WTYPE v;
  if (base == 0UL) {
    v = 0UL;
  } else if (base == BITS_PER_WORD) {
    v = ~0UL;
  } else {
    v = ( (1UL << base) | vread(list, base) ) - 1UL;
  }
  return v;
}

void put_delta (BitList *list, WTYPE value)
{
  if (value == 0UL) {
    //vwrite(list, 1, 1);
    put_gamma(list, 0);
  } else if (value == ~0UL) {
    put_gamma(list, BITS_PER_WORD);
  } else {
    WTYPE v = value+1;
    int base = 0;
    while ( (v >>= 1) != 0)
      base++;
    put_gamma(list, base);
    vwrite(list, base, value+1);
  }
}

WTYPE get_omega (BitList *list)
{
  WTYPE first_bit;
  WTYPE v = 1UL;
  while ( (first_bit = vread(list, 1)) == 1 ) {
    v = (1UL << v) | vread(list, v);
  }
  return (v == 0UL) ? ~0UL : v-1UL;
}

void put_omega (BitList *list, WTYPE value)
{
  // TODO: How to encode ~0UL ?
  value += 1UL;

  int sp = 0;
  int   stack_b[BIT_STACK_SIZE];
  WTYPE stack_v[BIT_STACK_SIZE];

  { stack_b[sp] = 1; stack_v[sp] = 0; sp++; }

  while (value > 1UL) {
    WTYPE v = value;
    int base = 0;
    while ( (v >>= 1) != 0)
      base++;
    assert(sp < BIT_STACK_SIZE);
    { stack_b[sp] = base+1; stack_v[sp] = value; sp++; }
    value = base;
  }

  while (sp > 0) {
    sp--;
    vwrite(list, stack_b[sp], stack_v[sp]);
  }
}

#define MAXFIB 100
static WTYPE fibv[MAXFIB] = {0};
static int   maxfibv = 0;
static void _calc_fibv(void)
{
  if (fibv[0] == 0) {
    fibv[0] = 1;
    fibv[1] = 2;
    int i;
    for (i = 2; i < MAXFIB; i++) {
      fibv[i] = fibv[i-2] + fibv[i-1];
      if (fibv[i] < fibv[i-1]) {
        maxfibv = i-1;
        break;
      }
    }
    assert(maxfibv > 0);
  }
}

WTYPE get_fib (BitList *list)
{
  _calc_fibv();
  WTYPE code = get_unary(list);
  WTYPE v = 0;
  int b = -1;
  do {
    // TODO: check pos and len
    b += (int) code+1;
    assert(b <= maxfibv);
    v += fibv[b];
  } while ( (code = get_unary(list)) != 0);
  return(v-1);
}

#define FIB_STACK_SIZE 256
void put_fib (BitList *list, WTYPE value)
{
  _calc_fibv();

  // Note that we're being very careful to allow ~0 to be encoded properly.

  int s = 0;
  while ( (s <= maxfibv) && (value >= (fibv[s]-1)) )
    s++;

  int sp = 0;
  int   stack_b[FIB_STACK_SIZE];
  WTYPE stack_v[FIB_STACK_SIZE];

  { stack_b[sp] = 2; stack_v[sp] = 3; sp++; }
  WTYPE v = value - fibv[--s] + 1;

  while (s-- > 0) {
    assert(sp < FIB_STACK_SIZE);
    if (v >= fibv[s]) {
      v -= fibv[s];
      { stack_b[sp] = 1; stack_v[sp] = 1; sp++; }
    } else {
      { stack_b[sp] = 1; stack_v[sp] = 0; sp++; }
    }
  }
  while (sp > 0) {
    sp--;
    vwrite(list, stack_b[sp], stack_v[sp]);
  }
}

WTYPE get_levenstein (BitList *list)
{
  WTYPE C = get_unary1(list);
  WTYPE v = 0;
  if (C > 0) {
    v = 1;
    int i;
    for (i = 1; i < C; i++) {
      v = (1UL << v) | vread(list, v);
    }
  }
  return(v);
}

void put_levenstein (BitList *list, WTYPE value)
{
  if (value == 0UL) {
    vwrite(list, 1, 0);
    return;
  }

  int sp = 0;
  int   stack_b[BIT_STACK_SIZE];
  WTYPE stack_v[BIT_STACK_SIZE];

  while (1) {
    WTYPE v = value;
    int base = 0;
    while ( (v >>= 1) != 0)
      base++;
    if (base == 0)
      break;
    assert(sp < BIT_STACK_SIZE);
    { stack_b[sp] = base; stack_v[sp] = value; sp++; }
    value = base;
  }

  put_unary1(list, sp+1);

  while (sp > 0) {
    sp--;
    vwrite(list, stack_b[sp], stack_v[sp]);
  }
}

WTYPE get_evenrodeh (BitList *list)
{
  WTYPE v = vread(list, 3);
  if (v > 3) {
    WTYPE first_bit;
    while ( (first_bit = vread(list, 1)) == 1UL ) {
      v = (1UL << (v-1UL)) | vread(list, v-1UL);
    }
  }
  return v;
}

void put_evenrodeh (BitList *list, WTYPE value)
{
  if (value <= 3UL) {
    vwrite(list, 3, value);
    return;
  }

  int sp = 0;
  int   stack_b[BIT_STACK_SIZE];
  WTYPE stack_v[BIT_STACK_SIZE];

  { stack_b[sp] = 1; stack_v[sp] = 0; sp++; }

  while (value > 3UL) {
    WTYPE v = value;
    int base = 1;
    while ( (v >>= 1) != 0)
      base++;
    assert(sp < BIT_STACK_SIZE);
    { stack_b[sp] = base; stack_v[sp] = value; sp++; }
    value = base;
  }

  while (sp > 0) {
    sp--;
    vwrite(list, stack_b[sp], stack_v[sp]);
  }
}

WTYPE get_baer (BitList *list, int k)
{
  assert(k >= -32);
  assert(k <= 32);
  int mk = (k < 0) ? -k : 0;

  WTYPE C = get_unary1(list);
  if (C < mk)
    return C;
  C -= mk;
  WTYPE v = (vread(list, 1) == 0UL)  ?  1UL  :  2UL + vread(list, 1);
  if (C > 0)
    v = (v << C)  +  ((1UL << (C+1UL)) - 2UL)  +  vread(list, C);
  v += mk;
  if (k > 0) {
    v = 1UL + ( ((v-1UL) << k) | vread(list, k) );
  }
  return (v-1UL);
}
void  put_baer (BitList *list, int k, WTYPE value)
{
  assert(k >= -32);
  assert(k <= 32);
  int mk = (k < 0) ? -k : 0;

  if (value < mk) {
    put_unary1(list, value);
    return;
  }
  WTYPE v = (k == 0)  ?  value+1UL
                      :  (k < 0)  ?  value-mk+1UL  :  1UL + (value >> k);
  WTYPE C = 0;
  WTYPE postword = 0;

  // This ensures ~0 is encoded correctly.
  if ( (k == 0) & (value >= 3) ) {
    if ((value & 1) == 0) { v = (value - 2UL) >> 1;  postword = 1UL; }
    else                  { v = (value - 1UL) >> 1; }
    C = 1;
  }

  while (v >= 4) {
    if ((v & 1) == 0) { v = (v - 2UL) >> 1; }
    else              { v = (v - 3UL) >> 1; postword |= (1UL << C); }
    C++;
  }

  put_unary1(list, C+mk);
  if (v == 1)
    vwrite(list, 1, 0);
  else
    vwrite(list, 2, v);
  if (C > 0)
    vwrite(list, C, postword);
  if (k > 0)
    vwrite(list, k, value);
}

WTYPE get_rice (BitList *list, int k)
{
  assert(k >= 0);
  assert(k <= BITS_PER_WORD);
  WTYPE v = get_unary(list);
  if (k > 0)
    v = (v << k) | vread(list, k);
  return v;
}
void  put_rice (BitList *list, int k, WTYPE value)
{
  assert(k >= 0);
  assert(k <= BITS_PER_WORD);
  if (k == 0) {
    put_unary(list, value);
  } else {
    WTYPE q = value >> k;
    WTYPE r = value - (q << k);
    put_unary(list, q);
    vwrite(list, k, r);
  }
}

WTYPE get_gamma_rice (BitList *list, int k)
{
  assert(k >= 0);
  assert(k <= BITS_PER_WORD);
  WTYPE v = get_gamma(list);
  if (k > 0)
    v = (v << k) | vread(list, k);
  return v;
}
void  put_gamma_rice (BitList *list, int k, WTYPE value)
{
  assert(k >= 0);
  assert(k <= BITS_PER_WORD);
  if (k == 0) {
    put_gamma(list, value);
  } else {
    WTYPE q = value >> k;
    WTYPE r = value - (q << k);
    put_gamma(list, q);
    vwrite(list, k, r);
  }
}

WTYPE get_golomb (BitList *list, WTYPE m)
{
  assert(m >= 1UL);

  WTYPE q = get_unary(list);
  if (m == 1UL)  return q;

  int base = 1;
  {
    WTYPE v = m-1UL;
    while (v >>= 1)  base++;
  }
  WTYPE threshold = (1UL << base) - m;

  WTYPE v = q * m;
  if (threshold == 0) {
    v += vread(list, base);
  } else {
    WTYPE first = vread(list, base-1);
    if (first >= threshold)
      first = (first << 1) + vread(list, 1) - threshold;
    v += first;
  }
  return v;
}
void  put_golomb (BitList *list, WTYPE m, WTYPE value)
{
  assert(m >= 1UL);
  if (m == 1UL) {
    put_unary(list, value);
    return;
  }

  int base = 1;
  {
    WTYPE v = m-1UL;
    while (v >>= 1)  base++;
  }
  WTYPE threshold = (1UL << base) - m;

  WTYPE q = value / m;
  WTYPE r = value - (q * m);
  assert(r >= 0);
  assert(r < m);
  assert(q*m+r == value);
  put_unary(list, q);
  if (r < threshold)
    vwrite(list, base-1, r);
  else
    vwrite(list, base, r + threshold);
}

WTYPE get_gamma_golomb (BitList *list, WTYPE m)
{
  assert(m >= 1UL);

  WTYPE q = get_gamma(list);
  if (m == 1UL)  return q;

  int base = 1;
  {
    WTYPE v = m-1UL;
    while (v >>= 1)  base++;
  }
  WTYPE threshold = (1UL << base) - m;

  WTYPE v = q * m;
  if (threshold == 0) {
    v += vread(list, base);
  } else {
    WTYPE first = vread(list, base-1);
    if (first >= threshold)
      first = (first << 1) + vread(list, 1) - threshold;
    v += first;
  }
  return v;
}
void  put_gamma_golomb (BitList *list, WTYPE m, WTYPE value)
{
  assert(m >= 1UL);
  if (m == 1UL) {
    put_gamma(list, value);
    return;
  }

  int base = 1;
  {
    WTYPE v = m-1UL;
    while (v >>= 1)  base++;
  }
  WTYPE threshold = (1UL << base) - m;

  WTYPE q = value / m;
  WTYPE r = value - (q * m);
  assert(r >= 0);
  assert(r < m);
  assert(q*m+r == value);
  put_gamma(list, q);
  if (r < threshold)
    vwrite(list, base-1, r);
  else
    vwrite(list, base, r + threshold);
}


#define QLOW  0
#define QHIGH 7
WTYPE get_adaptive_gamma_rice (BitList *list, int *kp)
{
  assert( (list != 0) && (kp != 0) );
  int k = *kp;
  if (k < 0) k = 0;
  if (k > BITS_PER_WORD) k = BITS_PER_WORD;

  WTYPE v;
  WTYPE q = get_gamma(list);
  if (k == 0) {
    v = q;
  } else {
    v = (q << k) | vread(list, k);
  }
  if ( (q <= QLOW ) && (k > 0            ) )  k--;
  if ( (q >= QHIGH) && (k < BITS_PER_WORD) )  k++;
  *kp = k;
  return v;
}
void put_adaptive_gamma_rice (BitList *list, int *kp, WTYPE value)
{
  assert( (list != 0) && (kp != 0) );
  int k = *kp;
  if (k < 0) k = 0;
  if (k > BITS_PER_WORD) k = BITS_PER_WORD;

  if ( (value == 0) && (k == 0) ) {
    vwrite(list, 1, 1);
    return;
  }

  WTYPE q = value >> k;
  put_gamma(list, q);
  if (k > 0) {
    WTYPE r = value - (q << k);
    vwrite(list, k, r);
  }
  if ( (q <= QLOW ) && (k > 0            ) )  k--;
  if ( (q >= QHIGH) && (k < BITS_PER_WORD) )  k++;
  *kp = k;
}

