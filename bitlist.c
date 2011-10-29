#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "bitlist.h"

static int verbose = 0;

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

static WTYPE call_get_sub(SV* code, BitList* list)
{
  SV* list_sv;

  dSP;
  ENTER;
  SAVETMPS;
  PUSHMARK(SP);

  list_sv = sv_newmortal();
  sv_setref_pv(list_sv, "Data::BitStream::XS", list);

  XPUSHs(list_sv);
  // args could go here

  PUTBACK;
  if (call_sv(code, G_SCALAR) != 1)
    croak("get sub should return one value");

  WTYPE v = POPl;

  SPAGAIN;
  PUTBACK;
  FREETMPS;
  LEAVE;
  return v;
}

static void call_put_sub(SV* code, BitList* list, WTYPE value)
{
  SV* list_sv;

  dSP;
  ENTER;
  SAVETMPS;
  PUSHMARK(SP);

  list_sv = sv_newmortal();
  sv_setref_pv(list_sv, "Data::BitStream::XS", list);

  XPUSHs(list_sv);
  XPUSHs(sv_2mortal(newSVuv(value)));

  PUTBACK;
  (void) call_sv(code, G_SCALAR);

  SPAGAIN;
  PUTBACK;
  FREETMPS;
  LEAVE;
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

  list->mode = eModeRW;
  list->file = 0;
  list->file_header = 0;
  list->file_header_lines = 0;

  list->is_writing = 0;
  switch (list->mode) {
    case eModeR:
    case eModeRO:
    case eModeA:    list->is_writing = 0;  break;
    case eModeW:
    case eModeWO:
    case eModeRW:   list->is_writing = 1;  break;
    default:        assert(0);
  }

  if (list->is_writing)
    write_open(list);
  else
    read_open(list);

  if (list->mode == eModeA)
    write_open(list);

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
    if (list->data == 0) {
      Perl_croak("failed to alloc %d bits", bits);
    } else if (newwords > oldwords) {
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
  if (list == 0) {
    Perl_croak("null object");
  } else {
    if (list->is_writing)
      write_close(list);
    if (list->data != 0)
      free(list->data);
    free(list);
  }
}

void dump(BitList *list)
{
  int words = NWORDS(list->len);
  int i;
  for (i = 0; i < words; i++) {
    fprintf(stderr, "%2d %08x\n", i, list->data[i]);
  }
}

int _set_len(BitList *list, int newlen)
{
  if ( (newlen < 0) || (newlen > list->maxlen) )
    Perl_croak("invalid length: %d", newlen);
  else
    list->len = newlen;
  return list->len;
}
int _set_pos(BitList *list, int newpos)
{
  assert(list != 0);
  if ( (newpos < 0) || (newpos > list->len) )
    Perl_croak("invalid position: %d", newpos);
  else
    list->pos = newpos;
  return list->pos;
}

void read_open(BitList *list)
{
  if (list->mode == eModeRO) {
    Perl_croak("read while stream opened writeonly");
    return;
  }
  if (list->is_writing)
    write_close(list);
  // TODO: file stuff
  assert(list->is_writing == 0);
}

void write_open(BitList *list)
{
  if (list->mode == eModeRO) {
    Perl_croak("write while stream opened readonly");
    return;
  }
  if (!list->is_writing) {
    list->is_writing = 1;
  }
  assert(list->is_writing == 1);
}

void write_close(BitList *list)
{
  if (list->is_writing) {
    list->is_writing = 0;
    list->pos = list->len;
    // TODO: file stuff
  }
  assert(list->is_writing == 0);
}

WTYPE sread(BitList *list, int bits)
{
  if ( (bits < 0) || (bits > BITS_PER_WORD) ) {
    Perl_croak("invalid bits: %d", bits);
    return 0UL;
  }
  WTYPE v = sreadahead(list, bits);
  list->pos += bits;
  return v;
}

WTYPE sreadahead(BitList *list, int bits)
{
  if ( (bits < 0) || (bits > BITS_PER_WORD) ) {
    Perl_croak("invalid bits: %d", bits);
    return 0UL;
  }

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

void swrite(BitList *list, int bits, WTYPE value)
{
  if (bits < 0) {
    Perl_croak("invalid bits: %d", bits);
    return;
  }

  int len = list->len;

  // expand the data if necessary
  expand_list(list, len+bits);

  if (value == 0) {
    list->len += bits;
    return;
  } else if (value == 1UL) {
    len += bits-1;
    bits = 1;
  }

  // Note that we allowed writing 0 and 1 with any number of bits.
  if ( (bits < 0) || (bits > BITS_PER_WORD) ) {
    Perl_croak("invalid bits: %d", bits);
    return;
  }

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
  assert( (list != 0) && (s != 0) );
  WTYPE word;
  int bits;
  while (*s != '\0') {
    word = 0;
    for (bits = 0;  (*s != 0) && (bits < BITS_PER_WORD);  bits++, s++) {
      word = (word << 1) | (*s != '0');
    }
    assert(bits > 0);
    swrite(list, bits, word);
  }
}

char* read_string(BitList *list, int bits)
{
  if (bits < 0) {
    Perl_croak("Invalid bits: %d", bits);
    return 0;
  }
  if (bits > (list->len - list->pos)) {
    Perl_croak("Short read");
    return 0;
  }
  char* buf = (char*) malloc(bits+1);
  if (buf == 0) {
    Perl_croak("alloc failure");
    return 0;
  }
  int pos = list->pos;
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
  //int bits = list->len;
  //int bytes = NWORDS(bits) * sizeof(WTYPE);
  int bytes = NBYTES(list->len);
  char* buf = (char*) malloc(bytes);
  if (buf != 0) {
    list->pos = 0;
    char* bptr = buf;
    int b;
    for (b = 0; b < bytes; b++) {
      *bptr++ = sread(list, 8);
    }
  }
  return buf;
}
void from_raw(BitList *list, char* str, int bits)
{
  if ( (str == 0) || (bits < 0) ) {
    Perl_croak("invalid input to from_raw");
    return;
  }
  resize(list, bits);
  if (bits > 0) {
    int bytes = NBYTES(bits);
    list->pos = 0;
    list->len = 0;
    char* bptr = str;
    while (bytes-- > 0) {
      swrite(list, 8, *bptr++);
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
  int maxpos = list->len - 1;

  // First word
  int wpos = pos / BITS_PER_WORD;
  int bpos = pos % BITS_PER_WORD;
  WTYPE *wptr = list->data + wpos;
  WTYPE word = (*wptr & (~0UL >> bpos)) << bpos;

  if (word == 0UL) {
    pos += (BITS_PER_WORD - bpos);
    while ( (*++wptr == 0UL) && (pos <= maxpos) )
       pos += BITS_PER_WORD;
    word = *wptr;
  }
  if (pos > maxpos) {
    Perl_croak("read off end of stream");
    return 0UL;
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
  /* We found a 1 in one of our valid words, this should be true */
  assert(pos <= maxpos);

  WTYPE v = pos - list->pos;
  list->pos = pos+1;
  assert(list->pos <= list->len);  /* double check we're not off the end */
  return v;
}

void put_unary (BitList *list, WTYPE value)
{
  // Simple way to do this:   swrite(list, value+1, 1);
  int len = list->len;
  int bits = value+1;

  expand_list(list, len+bits);

  len += value;

  int wpos = len / BITS_PER_WORD;
  int bpos = len % BITS_PER_WORD;

  list->data[wpos] |= (1UL << (MAXBIT - bpos));
  list->len = len + 1;
}

WTYPE get_unary1 (BitList *list)
{
  int pos = list->pos;
  int maxpos = list->len - 1;

  // First word
  int wpos = pos / BITS_PER_WORD;
  int bpos = pos % BITS_PER_WORD;
  WTYPE *wptr = list->data + wpos;
  WTYPE word = (bpos == 0) ? *wptr
                           : (*wptr << bpos) | (~0UL >> (BITS_PER_WORD - bpos));

  if (word == ~0UL) {
    pos += (BITS_PER_WORD - bpos);
    while ( (*++wptr == ~0UL) && (pos <= maxpos) )
       pos += BITS_PER_WORD;
    word = *wptr;
  }
  if (pos > maxpos) {
    Perl_croak("read off end of stream");
    return 0UL;
  }
  assert(word != ~0UL);

  while ( (word & (1UL << MAXBIT)) != 0UL ) {
    pos++;
    word <<= 1;
  }

  if (pos > maxpos) {
    Perl_croak("read off end of stream");
    return 0UL;
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
    swrite(list, BITS_PER_WORD, ~0UL);
    value -= BITS_PER_WORD;
  }
  if (value > 0)
    swrite(list, value, ~0UL);
  swrite(list, 1, 0UL);
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
  if (base > BITS_PER_WORD) {
    Perl_croak("Invalid base: %d", base);
    return 0UL;
  }
  WTYPE v;
  if (base == 0UL) {
    v = 0UL;
  } else if (base == BITS_PER_WORD) {
    v = ~0UL;
  } else {
    v = ( (1UL << base) | sread(list, base) ) - 1UL;
  }
  return v;
}

void put_gamma (BitList *list, WTYPE value)
{
  if (value == 0UL) {
    swrite(list, 1, 1);
  } else if (value == ~0UL) {
    put_unary(list, BITS_PER_WORD);
  } else {
    WTYPE v = value+1;
    int base = 1;
    while ( (v >>= 1) != 0)
      base++;
    swrite(list, base-1, 0UL);
    swrite(list, base, value+1);
  }
}

WTYPE get_delta (BitList *list)
{
  WTYPE base = get_gamma(list);
  if (base > BITS_PER_WORD) {
    Perl_croak("Invalid base: %d", base);
    return 0UL;
  }
  WTYPE v;
  if (base == 0UL) {
    v = 0UL;
  } else if (base == BITS_PER_WORD) {
    v = ~0UL;
  } else {
    v = ( (1UL << base) | sread(list, base) ) - 1UL;
  }
  return v;
}

void put_delta (BitList *list, WTYPE value)
{
  if (value == 0UL) {
    put_gamma(list, 0);
  } else if (value == ~0UL) {
    put_gamma(list, BITS_PER_WORD);
  } else {
    WTYPE v = value+1;
    int base = 0;
    while ( (v >>= 1) != 0)
      base++;
    put_gamma(list, base);
    swrite(list, base, value+1);
  }
}

WTYPE get_omega (BitList *list)
{
  WTYPE first_bit;
  WTYPE v = 1UL;
  while ( (first_bit = sread(list, 1)) == 1 ) {
    v = (1UL << v) | sread(list, v);
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
    swrite(list, stack_b[sp], stack_v[sp]);
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

void put_fib (BitList *list, WTYPE value)
{
  _calc_fibv();

  // We're constructing a big code backwards.  We fill in a word backwards,
  // then add it to a stack when full.  When done, we pop off each word from
  // the stack and write it.

  int s = 0;
  while ( (s <= maxfibv) && (value >= (fibv[s]-1)) )
    s++;

  int sp = 0;
  int   stack_b[BIT_STACK_SIZE];
  WTYPE stack_v[BIT_STACK_SIZE];

  // Note that we're being careful to allow ~0 to be encoded properly.
  WTYPE v = value - fibv[--s] + 1;

  // Current word we're constructing.  Trailing '11' filled in.
  WTYPE word = 3UL;
  WTYPE bits = 2;

  while (s-- > 0) {
    assert(sp < BIT_STACK_SIZE);
    if (v >= fibv[s]) {
      v -= fibv[s];
      word |= (1UL << bits);
    }
    bits++;
    if (bits >= BITS_PER_WORD) {
      stack_b[sp] = bits;  stack_v[sp] = word;  sp++;
      bits = 0;
      word = 0;
    }
  }
  if (bits)
    swrite(list, bits, word);
  while (sp-- > 0) {
    swrite(list, stack_b[sp], stack_v[sp]);
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
      v = (1UL << v) | sread(list, v);
    }
  }
  return(v);
}

void put_levenstein (BitList *list, WTYPE value)
{
  if (value == 0UL) {
    swrite(list, 1, 0);
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
    swrite(list, stack_b[sp], stack_v[sp]);
  }
}

WTYPE get_evenrodeh (BitList *list)
{
  WTYPE v = sread(list, 3);
  if (v > 3) {
    WTYPE first_bit;
    while ( (first_bit = sread(list, 1)) == 1UL ) {
      v = (1UL << (v-1UL)) | sread(list, v-1UL);
    }
  }
  return v;
}

void put_evenrodeh (BitList *list, WTYPE value)
{
  if (value <= 3UL) {
    swrite(list, 3, value);
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
    swrite(list, stack_b[sp], stack_v[sp]);
  }
}

WTYPE get_binword (BitList *list, int k)
{
  return sread(list, k);
}
void  put_binword (BitList *list, int k, WTYPE value)
{
  swrite(list, k, value);
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
  WTYPE v = (sread(list, 1) == 0UL)  ?  1UL  :  2UL + sread(list, 1);
  if (C > 0)
    v = (v << C)  +  ((1UL << (C+1UL)) - 2UL)  +  sread(list, C);
  v += mk;
  if (k > 0) {
    v = 1UL + ( ((v-1UL) << k) | sread(list, k) );
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
    swrite(list, 1, 0);
  else
    swrite(list, 2, v);
  if (C > 0)
    swrite(list, C, postword);
  if (k > 0)
    swrite(list, k, value);
}

typedef struct {
  int    maxhk;
  int    s [BITS_PER_WORD / 2];    // shift amount
  WTYPE  t [BITS_PER_WORD / 2];    // threshold
} bvzeta_map;

bvzeta_map bvzeta_map_cache[16] = {0};

static void bv_make_param_map(int k)
{
  assert(k >= 2);
  assert(k <= 15);
  bvzeta_map* bvm = &(bvzeta_map_cache[k]);
  if (bvm->maxhk == 0) {
    int maxh = (BITS_PER_WORD - 1) / k;
    assert(maxh < (BITS_PER_WORD/2));
    //int maxh = 0;
    //while ((k * (maxh+1)) < BITS_PER_WORD)  maxh++;
    int h;
    for (h = 0; h <= maxh; h++) {
      int hk = h * k;
      WTYPE interval = (1UL << (hk+k)) - (1UL << hk) - 1;
      WTYPE z = interval + 1UL;
      int s = 1;
      { WTYPE v = z; while ( (v >>= 1) != 0)  s++; } // ceil log2(z)
      assert(s >= 2);
      WTYPE thresh = (1UL << s) - z;
      bvm->s[h] = s;
      bvm->t[h] = thresh;
    }
    bvm->maxhk = maxh * k;
  }
}

WTYPE get_boldivigna (BitList *list, int k)
{
  assert(k >= 1);
  assert(k <= 15);  // You should use Delta codes for anything over 6.

  if (k == 1)  return get_gamma(list);

  bvzeta_map* bvm = &(bvzeta_map_cache[k]);
  if (bvm->maxhk == 0)
    bv_make_param_map(k);

  int maxh = bvm->maxhk / k;
  assert(maxh < (BITS_PER_WORD/2));

  WTYPE h = get_unary(list);
  if (h > maxh) return ~0UL;
  int   s = bvm->s[h];
  WTYPE t = bvm->t[h];
  assert(s >= 2);
  WTYPE first = sread(list, s-1);
  if (first >= t)
    first = (first << 1) + sread(list, 1) - t;

  WTYPE v = (1UL << (h*k)) - 1UL + first;   // -1 is to make 0 based
  return v;
}
void  put_boldivigna (BitList *list, int k, WTYPE value)
{
  assert(k >= 1);
  assert(k <= 15);  // You should use Delta codes for anything over 6.

  if (k == 1)  { put_gamma(list, value); return; }

  bvzeta_map* bvm = &(bvzeta_map_cache[k]);
  if (bvm->maxhk == 0)
    bv_make_param_map(k);

  int maxh = bvm->maxhk / k;
  assert(maxh < (BITS_PER_WORD/2));

  if (value == ~0UL)  { put_unary(list, maxh+1); return; }

  int maxhk = maxh * k;
  int hk = 0;
  while ( (hk < maxhk) && (value >= ((1UL << (hk+k))-1)) )  hk += k;
  int h = hk/k;
  assert(h <= maxh);
  int   s = bvm->s[h];
  WTYPE t = bvm->t[h];

  put_unary(list, h);
  WTYPE x = value - (1UL << hk) + 1UL;
  if (x < t)
    swrite(list, s-1, x);
  else
    swrite(list, s, x+t);
}

WTYPE get_rice (BitList *list, int k)
{
  assert(k >= 0);
  assert(k <= BITS_PER_WORD);
  WTYPE v = get_unary(list);
  if (k > 0)
    v = (v << k) | sread(list, k);
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
    swrite(list, k, r);
  }
}

WTYPE get_gamma_rice (BitList *list, int k)
{
  assert(k >= 0);
  assert(k <= BITS_PER_WORD);
  WTYPE v = get_gamma(list);
  if (k > 0)
    v = (v << k) | sread(list, k);
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
    swrite(list, k, r);
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
    v += sread(list, base);
  } else {
    WTYPE first = sread(list, base-1);
    if (first >= threshold)
      first = (first << 1) + sread(list, 1) - threshold;
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
    swrite(list, base-1, r);
  else
    swrite(list, base, r + threshold);
}

WTYPE get_golomb_sub (BitList *list, SV* code, WTYPE m)
{
  assert(m >= 1UL);
  assert(code != 0);

  //WTYPE q = get_unary(list);
  WTYPE q = call_get_sub(code, list);
  if (m == 1UL)  return q;

  int base = 1;
  {
    WTYPE v = m-1UL;
    while (v >>= 1)  base++;
  }
  WTYPE threshold = (1UL << base) - m;

  WTYPE v = q * m;
  if (threshold == 0) {
    v += sread(list, base);
  } else {
    WTYPE first = sread(list, base-1);
    if (first >= threshold)
      first = (first << 1) + sread(list, 1) - threshold;
    v += first;
  }
  return v;
}
void  put_golomb_sub (BitList *list, SV* code, WTYPE m, WTYPE value)
{
  assert(m >= 1UL);
  assert(code != 0);

  if (m == 1UL) {
    //put_unary(list, value);
    call_put_sub(code, list, value);
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
  //put_unary(list, q);
  call_put_sub(code, list, q);
  if (r < threshold)
    swrite(list, base-1, r);
  else
    swrite(list, base, r + threshold);
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
    v += sread(list, base);
  } else {
    WTYPE first = sread(list, base-1);
    if (first >= threshold)
      first = (first << 1) + sread(list, 1) - threshold;
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
    swrite(list, base-1, r);
  else
    swrite(list, base, r + threshold);
}


#define QLOW  0
#define QHIGH 7
WTYPE get_adaptive_gamma_rice (BitList *list, int *kp)
{
  assert( (list != 0) && (kp != 0) );
  int k = *kp;
  assert(k >= 0);
  assert(k <= BITS_PER_WORD);

  WTYPE v;
  WTYPE q = get_gamma(list);
  if (k == 0) {
    v = q;
  } else {
    v = (q << k) | sread(list, k);
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
  assert(k >= 0);
  assert(k <= BITS_PER_WORD);

  if ( (value == 0) && (k == 0) ) {
    swrite(list, 1, 1);
    return;
  }

  WTYPE q = value >> k;
  put_gamma(list, q);
  if (k > 0) {
    WTYPE r = value - (q << k);
    swrite(list, k, r);
  }
  if ( (q <= QLOW ) && (k > 0            ) )  k--;
  if ( (q >= QHIGH) && (k < BITS_PER_WORD) )  k++;
  *kp = k;
}

