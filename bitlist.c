#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "bitlist.h"

static int verbose = 0;
#define BITS_PER_WORD (8 * sizeof(WTYPE))
#define MAXBIT        (BITS_PER_WORD-1)
#define NWORDS(bits)  ( (bits+BITS_PER_WORD-1) / BITS_PER_WORD )

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

  list->len = 0;
  list->pos = 0;
  list->maxlen = 0;
  list->data = 0;

  if (bits > 0)
    (void) reserve_bits(list, bits);

  return list;
}

int reserve_bits(BitList *list, int bits)
{
  if (list->maxlen < bits) {
    if (verbose)
     fprintf(stderr, "DBV: expanding from %d to %d bits\n", list->maxlen, bits);
    int oldwords = NWORDS(list->maxlen);
    int newwords = NWORDS(bits);
    WTYPE* newdata = (WTYPE*) calloc(newwords, sizeof(WTYPE));
    if (list->data != 0) {
      memcpy(newdata, list->data, oldwords * sizeof(WTYPE));
      free(list->data);
    }
    list->data = newdata;
    list->maxlen = bits;
  }
  return list->maxlen;
}
int resize(BitList *list, int bits)
{
  if (bits == 0) {
    // erase everything
    if (list->data != 0) {
      free(list->data);
      list->data = 0;
    }
    list->maxlen = 0;
    list->len = 0;
    list->pos = 0;
  } else if (bits < list->maxlen) {
    // shrink
    int words = NWORDS(bits);
    WTYPE* newdata = (WTYPE*) calloc(words, sizeof(WTYPE));
    if (list->data != 0) {
      memcpy(newdata, list->data, words * sizeof(WTYPE));
      free(list->data);
    }
    list->data = newdata;
    list->maxlen = bits;
  } else {
    reserve_bits(list, bits);
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

  WTYPE v = 0;
  int pos = list->pos;

  // Handle readahead possibility
  int shift = (pos+bits) - list->len;

  if (shift > 0) {
    bits -= shift;
  }

  //fprintf(stderr, "reading %d bits at pos %d\n", bits, pos);
#if 0
  while (bits > 0) {
    int wpos = pos / BITS_PER_WORD;
    int bpos = pos % BITS_PER_WORD;
    WTYPE word = list->data[wpos];
    WTYPE bit  = (word >> ( MAXBIT - bpos)) & 1;
    v = (v << 1) | bit;
    bits--;
    pos++;
    //fprintf(stderr, "  read %d from word %d bit %d\n",
    //        bit, wpos, (BITS_PER_WORD-1) - bpos);
  }
#else
  int wpos = pos / BITS_PER_WORD;
  int bpos = pos % BITS_PER_WORD;
  if (bpos <= (BITS_PER_WORD-bits)) {
    v = (list->data[wpos] >> (BITS_PER_WORD-bpos-bits))
        & (~0UL >> (BITS_PER_WORD-bits));
    pos += bits;
  } else {
    // TODO make faster
    while (bits > 0) {
      wpos = pos / BITS_PER_WORD;
      bpos = pos % BITS_PER_WORD;
      WTYPE word = list->data[wpos];
      WTYPE bit  = (word >> ( MAXBIT - bpos)) & 1;
      v = (v << 1) | bit;
      bits--;
      pos++;
      //fprintf(stderr, "  read %d from word %d bit %d\n",
      //        bit, wpos, (BITS_PER_WORD-1) - bpos);
    }
  }
#endif
  list->pos = pos;

  if (shift > 0) {
    v <<= shift;
  }

  return v;
}

void vwrite(BitList *list, int bits, WTYPE value)
{
  int len = list->len;

  // TODO return undef or die or expand
  if ( (len + bits) > list->maxlen) {
    resize(list, 1.15 * (len+bits+2048) );
  }

  if (value == 0) {
    list->len += bits;
    return;
  }
  if (value == 1) {
    len += bits-1;
    bits = 1;
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
#else
  int wpos = len / BITS_PER_WORD;
  int bpos = len % BITS_PER_WORD;
  // Write into this word
  if (bpos > 0) {
    int endpos = ((bpos + bits) > BITS_PER_WORD) ? BITS_PER_WORD : bpos + bits;
    int bits_to_write = endpos - bpos;
    WTYPE v = value >> (bits - bits_to_write);
    list->data[wpos] |= v << (BITS_PER_WORD - endpos);
    len += bits_to_write;
    bits -= bits_to_write;
  }
  // Write into the next word if needed
  if (bits > 0) {
    int wpos = len / BITS_PER_WORD;
    WTYPE v = value & (~0UL >> (BITS_PER_WORD - bits));
    list->data[wpos] |= v << (BITS_PER_WORD - bits);
    len += bits;
  }
#endif

  list->len = len;
}

WTYPE get_unary (BitList *list)
{
  int pos = list->pos;
  WTYPE word;

  // First word
  int wpos = pos / BITS_PER_WORD;
  int bpos = pos % BITS_PER_WORD;
  word = (list->data[wpos] & (~0UL >> bpos)) << bpos;

  if (word == 0) {
    pos += (BITS_PER_WORD - bpos);
    while ( list->data[ pos / BITS_PER_WORD ] == 0 )
       pos += BITS_PER_WORD;
    word = list->data[ pos / BITS_PER_WORD ];
  }
  assert(word != 0);

  while ( ((word >> MAXBIT) & 1) == 0 ) {
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

  if ( (len + bits) > list->maxlen) {
    resize(list, 1.15 * (len+bits+2048) );
  }

  len += value;

  int wpos = len / BITS_PER_WORD;
  int bpos = len % BITS_PER_WORD;

  list->data[wpos] |= (1UL << (MAXBIT - bpos));
  len++;
  list->len = len;
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

void put_string(BitList *list, char* s)
{
  // TODO: better implementation
  while (*s != '\0') {
    assert( (*s == '0') || (*s == '1') );
    vwrite(list, 1, *s != '0');
    s++;
  }
}
