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

/* This is for debugging */
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

static WTYPE call_get_sub(SV* self, SV* code, BitList* list)
{
  dSP;                               /* Local copy of stack pointer         */
  int count;
  WTYPE v;

  ENTER;                             /* Start wrapper                       */
  SAVETMPS;                          /* Start (2)                           */

  PUSHMARK(SP);                      /* Start args: note our SP             */
  XPUSHs(self);                      /*    our stream                       */
  PUTBACK;                           /* End args:   set global SP to ours   */

  count = call_sv(code, G_SCALAR);   /* Call the sub                        */
  SPAGAIN;                           /* refresh local stack pointer         */

  if (count != 1)
    croak("get sub should return one value");

  v = POPul;                         /* Get the returned value              */

  /* TODO:
   *   Something isn't right here -- the stack is messed up if I do the
   *   PUTBACK, but skipping it is wrong.
   */
#if 0
  PUTBACK;                           /* let global SP know what we did      */
#endif
  FREETMPS;                          /* End wrapper                         */
  LEAVE;                             /* End (2)                             */
  return v;
}

static void call_put_sub(SV* self, SV* code, BitList* list, WTYPE value)
{
  dSP;
  int count;

  ENTER;
  SAVETMPS;

  PUSHMARK(SP);
  XPUSHs(self);
  XPUSHs(sv_2mortal(newSVuv(value)));
  PUTBACK;

  count = call_sv(code, G_VOID);
  SPAGAIN;

  PUTBACK;
  FREETMPS;
  LEAVE;
}


BitList *new(
  FileMode mode,
  char*    file,
  int      fheaderlines,
  int      initial_bits
)
{
  BitList *list = (BitList *) malloc(sizeof (BitList));
  if (list == 0)
    return list;

  list->data = 0;
  list->pos = 0;
  list->len = 0;
  list->maxlen = 0;

  list->mode = mode;
  list->file = file;
  list->file_header = 0;
  list->file_header_lines = fheaderlines;

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

  if (initial_bits > 0)
    (void) resize(list, initial_bits);

  return list;
}

int resize(BitList *list, int bits)
{
  assert(bits >= 0);
  if (bits == 0) {
    /* erase everything */
    if (list->data != 0) {
      free(list->data);
      list->data = 0;
    }
  } else {
    /* Grow or shrink */
    int oldwords = NWORDS(list->maxlen);
    int newwords = NWORDS(bits);
    list->data = (WTYPE*) realloc(list->data, newwords * sizeof(WTYPE));
    if (list->data == 0) {
      croak("failed to alloc %d bits", bits);
    } else if (newwords > oldwords) {
      /* Zero out any new allocated space */
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
    croak("null object");
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
    fprintf(stderr, "%2d %08lx\n", i, list->data[i]);
  }
}

int _set_len(BitList *list, int newlen)
{
  if ( (newlen < 0) || (newlen > list->maxlen) )
    croak("invalid length: %d", newlen);
  else
    list->len = newlen;
  return list->len;
}
int _set_pos(BitList *list, int newpos)
{
  assert(list != 0);
  if ( (newpos < 0) || (newpos > list->len) )
    croak("invalid position: %d", newpos);
  else
    list->pos = newpos;
  return list->pos;
}

void read_open(BitList *list)
{
  if (list->mode == eModeWO) {
    croak("read while stream opened writeonly");
    return;
  }
  if (list->is_writing)
    write_close(list);
  if (list->file != 0) {
    unsigned long bits;
    FILE* fh = fopen(list->file, "r");
    if (!fh) {
      croak("Cannot open file %s", list->file);
      return;
    }
    /* Read in their header lines */
    if (list->file_header_lines > 0) {
      int hline;
      int maxbytes = 512 * list->file_header_lines;
      int nbytes = 0;
      char* hbuf = (char*) malloc(nbytes);
      char* hptr = hbuf;
      for (hline = 0; hline < list->file_header_lines; hline++) {
        char* fresult;
        int len;
        if (nbytes >= maxbytes) {
          croak("Overflow reading header line %d/%d", hline, list->file_header_lines);
          fclose(fh);
          return;
        }
        fresult = fgets(hptr, maxbytes - nbytes, fh);
        len = strlen(hptr);
        if ( !fresult || feof(fh) || (len == 0) || (hptr[len-1] != '\n') ) {
          croak("Error reading header line %d/%d", hline, list->file_header_lines);
          fclose(fh);
          return;
        }
        hptr += len;
        nbytes += len;
      }
      hbuf = (char*) realloc(hbuf, nbytes+1);
      if (list->file_header != 0)
        free(list->file_header);
      list->file_header = hbuf;
    }
    /* Read the number of bits */
    if (fscanf(fh, "%lu\n", &bits) != 1) {
      croak("Cannot read number of bits from file");
      fclose(fh);
      return;
    }
    /* Read data */
    list->pos = 0;
    list->len = 0;
    {
      size_t total_bytes = 0;
      char* buf = (char*) malloc(16384 * sizeof(char));
      while (!feof(fh)) {
        char* bptr = buf;
        size_t bytes = fread(buf, sizeof(char), 16384, fh);
        total_bytes += bytes;
        while (bytes-- > 0) {
          swrite(list, 8, *bptr++);
        }
      }
      free(buf);
      if (total_bytes != NBYTES(bits)) {
        croak("Read %d bytes, expected %lu", total_bytes, NBYTES(bits));
        fclose(fh);
        return;
      }
      list->len = bits;
    }
    fclose(fh);
  }
  assert(list->is_writing == 0);
}

void write_open(BitList *list)
{
  if (list->mode == eModeRO) {
    croak("write while stream opened readonly");
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
    if (list->file != 0) {
      FILE* fh;
      char* buf = to_raw(list);
      if (buf == 0)
        return;
      fh = fopen(list->file, "w");
      if (!fh) {
        croak("Cannot open file %s", list->file);
      } else {
        if (list->file_header != 0)
          fprintf(fh, "%s\n", list->file_header);
        fprintf(fh, "%d\n", list->len);
        fwrite(buf, 1, NBYTES(list->len), fh);
      }
      free(buf);
      fclose(fh);
    }
    list->is_writing = 0;
    list->pos = list->len;
  }
  assert(list->is_writing == 0);
}

WTYPE sread(BitList *list, int bits)
{
  WTYPE v;
  int wpos, bpos, wlen;

  if ( (bits <= 0) || (bits > BITS_PER_WORD) ) {
    croak("invalid bits: %d", bits);
    return W_ZERO;
  }
  assert( (list->pos + bits) <= list->len );
#if 0
  WTYPE v = sreadahead(list, bits);
#else
  wpos = list->pos / BITS_PER_WORD;
  bpos = list->pos % BITS_PER_WORD;
  wlen = BITS_PER_WORD - bits;

  if (bpos <= wlen) {  /* single-word read */
    v = (list->data[wpos] >> (wlen-bpos)) & (W_FFFF >> wlen);
  } else {             /* double-word read */
    int bits1 = BITS_PER_WORD - bpos;
    int bits2 = bits - bits1;
    v =   ( (list->data[wpos+0] & (W_FFFF >> bpos)) << bits2 )
        | ( list->data[wpos+1] >> (BITS_PER_WORD - bits2) );
  }
#endif
  list->pos += bits;
  return v;
}

WTYPE sreadahead(BitList *list, int bits)
{
  WTYPE v = 0;
  int pos = list->pos;
  int shift, wpos, bpos, wlen;

  if ( (bits <= 0) || (bits > BITS_PER_WORD) ) {
    croak("invalid bits: %d", bits);
    return W_ZERO;
  }

  /* Readahead can read past the end of the data, and requires that we fill
   * in with zeros.  We could force the data to always have BITS_PER_WORD of
   * empty space, or we can shift things here. */
  shift = (pos+bits) - list->len;

  if (shift > 0) {
    bits -= shift;
  }

  wpos = pos / BITS_PER_WORD;
  bpos = pos % BITS_PER_WORD;
  wlen = BITS_PER_WORD - bits;

  if (bpos <= wlen) {  /* single-word read */
    v = (list->data[wpos] >> (wlen-bpos)) & (W_FFFF >> wlen);
  } else {             /* double-word read */
    int bits1 = BITS_PER_WORD - bpos;
    int bits2 = bits - bits1;
    v =   ( (list->data[wpos+0] & (W_FFFF >> bpos)) << bits2 )
        | ( list->data[wpos+1] >> (BITS_PER_WORD - bits2) );
  }

  if (shift > 0) {
    v <<= shift;
  }

  return v;
}

void swrite(BitList *list, int bits, WTYPE value)
{
  int len = list->len;
  int wpos, bpos, wlen;

  if (bits <= 0) {
    croak("invalid bits: %d", bits);
    return;
  }

  /* expand the data if necessary */
  expand_list(list, len+bits);

  if (value == 0) {
    list->len += bits;
    return;
  } else if (value == W_ONE) {
    len += bits-1;
    bits = 1;
  }

  /* We allowed writing 0 and 1 with any number of positive bits. */
  if (bits > BITS_PER_WORD) {
    croak("invalid bits: %d", bits);
    return;
  }

  /* mask value if needed */
  if (bits < BITS_PER_WORD) {
    value &= (W_FFFF >> (BITS_PER_WORD - bits));
    assert(value < (W_ONE << bits));
  }

#if 0
  /* Simple write */
  while (bits > 0) {
    int wpos = len / BITS_PER_WORD;
    int bpos = len % BITS_PER_WORD;
    WTYPE* wptr = list->data + wpos;
    WTYPE bit = (value >> (bits-1)) & 1;
    *wptr |= (bit << ( MAXBIT - bpos));
    /* fprintf(stderr, "w%3d/%2d=%d [%s]\n", wpos, (BITS_PER_WORD-1) - bpos, bit, word_to_bin(*wptr)); */
    bits--;
    len++;
  }
  list->len = len;
  return;
#endif

  wpos = len / BITS_PER_WORD;
  bpos = len % BITS_PER_WORD;
  wlen = BITS_PER_WORD - bits;

  if (bpos <= wlen) {  /* single-word write */
    list->data[wpos] |= (value & (W_FFFF >> wlen)) << (wlen-bpos);
  } else {             /* double-word write */
    int first_bits = BITS_PER_WORD - bpos;
    int wlen       = BITS_PER_WORD - (bits - first_bits);
    list->data[wpos++] |=  value >> (bits - first_bits);
    list->data[wpos] |= (value & (W_FFFF >> wlen)) << (wlen-0);
  }

  list->len = len + bits;
}

void put_string(BitList *list, char* s)
{
  /* Write words.  Reasonably fast. */
  WTYPE word;
  int bits;

  assert( (list != 0) && (s != 0) );
  while (*s != '\0') {
    word = 0;
    for (bits = 0;  (*s != 0) && (bits < BITS_PER_WORD);  bits++, s++) {
      word = (word << 1) | (*s != '0');
    }
    assert(bits > 0);
    swrite(list, bits, word);
  }
}

#if 0
static int  str_map_init = 0;
static char str_map[256][8];
#endif

char* read_string(BitList *list, int bits)
{
  char* buf;
  int pos = list->pos;
  assert(bits >= 0);
  assert( list->pos < list->len );
  assert (bits <= (list->len - list->pos));
  buf = (char*) malloc(bits+1);
  if (buf == 0) {
    croak("alloc failure");
    return 0;
  }
#if 0
  /* Simple code */
  int b;
  for (b = 0; b < bits; b++) {
    int wpos = pos / BITS_PER_WORD;
    int bpos = pos % BITS_PER_WORD;
    if ( ((list->data[wpos] << bpos) & (W_ONE << MAXBIT)) == 0 )
      buf[b] = '0';
    else
      buf[b] = '1';
    pos++;
  }
  list->pos = pos;
#endif
#if 1
  {
    /* Much better, but could still be sped up. */
    int wpos = pos / BITS_PER_WORD;
    int bpos = pos % BITS_PER_WORD;
    WTYPE word = list->data[wpos] << bpos;
    char*  bptr = buf;
    int b = bits;
    while (b-- > 0) {
      *bptr++ = ((word & (W_ONE << MAXBIT)) == 0) ? '0' : '1';
      word <<= 1;
      if (++bpos >= BITS_PER_WORD) {
        word = list->data[++wpos];
        bpos = 0;
        while (b >= BITS_PER_WORD) {
          if (word == W_ZERO) {
            memset(bptr, '0', BITS_PER_WORD);
            bptr += BITS_PER_WORD;
            b -= BITS_PER_WORD;
            word = list->data[++wpos];
          } else if (word == W_FFFF) {
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
  }
#endif
  buf[bits] = '\0';
  return buf;
}

char* to_raw(BitList *list)
{
  int bytes = NBYTES(list->len);
  char* buf = (char*) malloc(bytes);
  if (buf != 0) {
    char* bptr = buf;
    int b;
    list->pos = 0;
    for (b = 0; b < bytes; b++) {
      *bptr++ = sreadahead(list, 8);
      list->pos += 8;
    }
  }
  return buf;
}
void from_raw(BitList *list, char* str, int bits)
{
  if ( (str == 0) || (bits < 0) ) {
    croak("invalid input to from_raw");
    return;
  }
  resize(list, bits);
  if (bits > 0) {
    int bytes = NBYTES(bits);
    char* bptr = str;
    list->pos = 0;
    list->len = 0;
    while (bytes-- > 0) {
      swrite(list, 8, *bptr++);
    }
    list->len = bits;
  }
}

void xput_stream (BitList *list, BitList *src)
{
  if (src->len <= 0)
    return;

  /* rewind_for_read(src) */
  if (src->is_writing)
    write_close(src);
  src->pos = 0;

  assert(list->is_writing);
  assert(!src->is_writing);

  expand_list(list, list->len + src->len);

  if (list->len == 0) {
    /* Copy direct.  Must copy whole words. */
    memcpy(list->data, src->data, sizeof(WTYPE) * NWORDS(src->len));
    list->len = src->len;
    src->pos = src->len;
  } else {
    /* sread/swrite */
    int bits = src->len;
    while (bits >= BITS_PER_WORD) {
      swrite(list, BITS_PER_WORD,  sread(src, BITS_PER_WORD)  );
      bits -= BITS_PER_WORD;
    }
    if (bits > 0)
      swrite(list, bits,  sread(src, bits)  );
  }
}



/*******************************************************************************
 *
 *                                      CODES
 *
 ******************************************************************************/

WTYPE get_unary (BitList *list)
{
  int pos, maxpos, wpos, bpos;
  WTYPE word, v;
  WTYPE *wptr;

  assert( list->pos < list->len );
  pos = list->pos;
  maxpos = list->len - 1;

  /* First word */
  wpos = pos / BITS_PER_WORD;
  bpos = pos % BITS_PER_WORD;
  wptr = list->data + wpos;
  word = (*wptr & (W_FFFF >> bpos)) << bpos;

  if (word == W_ZERO) {
    pos += (BITS_PER_WORD - bpos);
    while ( (*++wptr == W_ZERO) && (pos <= maxpos) )
       pos += BITS_PER_WORD;
    word = *wptr;
  }
  if (pos > maxpos) {
    croak("read off end of stream");
    return W_ZERO;
  }
  assert(word != 0);

#if 0
  #if sizeof(WTYPE) >= 8
    if ((word & W_CONST(0xFFFFFFFF00000000))==W_ZERO) {pos+=32; word<<=32;}
    if ((word & W_CONST(0xFFFF000000000000))==W_ZERO) {pos+=16; word<<=16;}
    if ((word & W_CONST(0xFF00000000000000))==W_ZERO) {pos+= 8; word<<= 8;}
  #else
    if ((word & W_CONST(0xFFFF0000))==W_ZERO) {pos+=16; word<<=16;}
    if ((word & W_CONST(0xFF000000))==W_ZERO) {pos+= 8; word<<= 8;}
  #endif
#endif
  while ( ((word >> MAXBIT) & W_ONE) == W_ZERO ) {
    pos++;
    word <<= 1;
  }
  /* We found a 1 in one of our valid words, this should be true */
  assert(pos <= maxpos);

  v = pos - list->pos;
  list->pos = pos+1;
  assert(list->pos <= list->len);  /* double check we're not off the end */
  return v;
}

void put_unary (BitList *list, WTYPE value)
{
  int len, bits, wpos, bpos;
  /* Simple way to do this:   swrite(list, value+1, 1); */
  len = list->len;
  bits = value+1;

  expand_list(list, len+bits);

  len += value;

  wpos = len / BITS_PER_WORD;
  bpos = len % BITS_PER_WORD;

  list->data[wpos] |= (W_ONE << (MAXBIT - bpos));
  list->len = len + 1;
}

WTYPE get_unary1 (BitList *list)
{
  int pos, maxpos, wpos, bpos;
  WTYPE word, v;
  WTYPE *wptr;
  assert( list->pos < list->len );
  pos = list->pos;
  maxpos = list->len - 1;

  /* First word */
  wpos = pos / BITS_PER_WORD;
  bpos = pos % BITS_PER_WORD;
  wptr = list->data + wpos;
  word = (bpos == 0) ? *wptr
                     : (*wptr << bpos) | (W_FFFF >> (BITS_PER_WORD - bpos));

  if (word == W_FFFF) {
    pos += (BITS_PER_WORD - bpos);
    while ( (*++wptr == W_FFFF) && (pos <= maxpos) )
       pos += BITS_PER_WORD;
    word = *wptr;
  }
  if (pos > maxpos) {
    croak("read off end of stream");
    return W_ZERO;
  }
  assert(word != W_FFFF);

  while ( (word & (W_ONE << MAXBIT)) != W_ZERO ) {
    pos++;
    word <<= 1;
  }

  if (pos > maxpos) {
    croak("read off end of stream");
    return W_ZERO;
  }

  v = pos - list->pos;
  list->pos = pos+1;
  return v;
}

void put_unary1 (BitList *list, WTYPE value)
{
  int len = list->len;
  int bits = value+1;

  int wpos = len / BITS_PER_WORD;
  int bpos = len % BITS_PER_WORD;
  int first_bits = BITS_PER_WORD - bpos;

  expand_list(list, len+value+1);

  if ( (bpos > 0) && (first_bits <= value) ) {
    list->data[wpos++] |= (W_FFFF >> bpos);
    bpos = 0;
    value -= first_bits;
  }
  while (value > BITS_PER_WORD) {
    list->data[wpos++] = W_FFFF;
    value -= BITS_PER_WORD;
  }
  if (value > 0)
    list->data[wpos] |= ( (W_FFFF << (BITS_PER_WORD-value)) >> bpos);

  list->len = len + bits;
}

WTYPE get_gamma (BitList *list)
{
  WTYPE base, v;
  assert( list->pos < list->len );
  base = get_unary(list);
  if (base > BITS_PER_WORD) {
    croak("Invalid base: %lu", base);
    return W_ZERO;
  }
  if (base == W_ZERO) {
    v = W_ZERO;
  } else if (base == BITS_PER_WORD) {
    v = W_FFFF;
  } else {
    v = ( (W_ONE << base) | sread(list, base) ) - W_ONE;
  }
  return v;
}

void put_gamma (BitList *list, WTYPE value)
{
  if (value == W_ZERO) {
    swrite(list, 1, 1);
  } else if (value == W_FFFF) {
    put_unary(list, BITS_PER_WORD);
  } else {
    WTYPE v = value+1;
    int base = 1;
    while ( (v >>= 1) != 0)
      base++;
    swrite(list, base-1, W_ZERO);
    swrite(list, base, value+1);
  }
}

WTYPE get_delta (BitList *list)
{
  WTYPE base, v;
  assert( list->pos < list->len );
  base = get_gamma(list);
  if (base > BITS_PER_WORD) {
    croak("Invalid base: %lu", base);
    return W_ZERO;
  }
  if (base == W_ZERO) {
    v = W_ZERO;
  } else if (base == BITS_PER_WORD) {
    v = W_FFFF;
  } else {
    v = ( (W_ONE << base) | sread(list, base) ) - W_ONE;
  }
  return v;
}

void put_delta (BitList *list, WTYPE value)
{
  if (value == W_ZERO) {
    put_gamma(list, 0);
  } else if (value == W_FFFF) {
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
  WTYPE v = W_ONE;
  assert( list->pos < list->len );
  while ( (first_bit = sread(list, 1)) == 1 ) {
    v = (W_ONE << v) | sread(list, v);
  }
  return (v == W_ZERO) ? W_FFFF : v-W_ONE;
}

void put_omega (BitList *list, WTYPE value)
{
  int sp = 0;
  int   stack_b[BIT_STACK_SIZE];
  WTYPE stack_v[BIT_STACK_SIZE];

  /* TODO: How to encode W_FFFF (~0) ? */
  value += W_ONE;
  { stack_b[sp] = 1; stack_v[sp] = 0; sp++; }

  while (value > W_ONE) {
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
    int i;
    fibv[0] = 1;
    fibv[1] = 2;
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
  int b;
  WTYPE code, v;

  assert( list->pos < list->len );
  _calc_fibv();
  code = get_unary(list);
  v = 0;
  b = -1;
  do {
    /* TODO: check pos and len */
    b += (int) code+1;
    assert(b <= maxfibv);
    v += fibv[b];
  } while ( (code = get_unary(list)) != 0);
  return(v-1);
}

void put_fib (BitList *list, WTYPE value)
{
  int   sp = 0;
  int   stack_b[BIT_STACK_SIZE];
  WTYPE stack_v[BIT_STACK_SIZE];
  int s, bits;
  WTYPE v, word;

  _calc_fibv();

  /* We're constructing a big code backwards.  We fill in a word backwards,
   * then add it to a stack when full.  When done, we pop off each word from
   * the stack and write it. */

  s = 0;
  while ( (s <= maxfibv) && (value >= (fibv[s]-1)) )
    s++;

  /* Note that we're being careful to allow ~0 to be encoded properly. */
  v = value - fibv[--s] + 1;

  /* Current word we're constructing.  Trailing '11' filled in. */
  word = W_CONST(3);
  bits = 2;

  while (s-- > 0) {
    assert(sp < BIT_STACK_SIZE);
    if (v >= fibv[s]) {
      v -= fibv[s];
      word |= (W_ONE << bits);
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
  WTYPE C, v;
  assert( list->pos < list->len );
  C = get_unary1(list);
  v = 0;
  if (C > 0) {
    int i;
    v = 1;
    for (i = 1; i < C; i++) {
      v = (W_ONE << v) | sread(list, v);
    }
  }
  return(v);
}

void put_levenstein (BitList *list, WTYPE value)
{
  int   sp = 0;
  int   stack_b[BIT_STACK_SIZE];
  WTYPE stack_v[BIT_STACK_SIZE];

  if (value == W_ZERO) {
    swrite(list, 1, 0);
    return;
  }

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
  WTYPE v, first_bit;
  assert( list->pos < list->len );
  v = sread(list, 3);
  if (v > 3) {
    while ( (first_bit = sread(list, 1)) == W_ONE ) {
      v = (W_ONE << (v-W_ONE)) | sread(list, v-W_ONE);
    }
  }
  return v;
}

void put_evenrodeh (BitList *list, WTYPE value)
{
  int   sp = 0;
  int   stack_b[BIT_STACK_SIZE];
  WTYPE stack_v[BIT_STACK_SIZE];

  if (value <= W_CONST(3)) {
    swrite(list, 3, value);
    return;
  }

  { stack_b[sp] = 1; stack_v[sp] = 0; sp++; }

  while (value > W_CONST(3)) {
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
  int mk;
  WTYPE C, v;

  assert(k >= -32);
  assert(k <= 32);
  mk = (k < 0) ? -k : 0;

  C = get_unary1(list);
  if (C < mk)
    return C;
  C -= mk;
  v = (sread(list, 1) == W_ZERO)  ?  W_ONE
                                  :  W_CONST(2) + sread(list, 1);
  if (C > 0)
    v = (v << C)  +  ((W_ONE << (C+W_ONE)) - W_CONST(2))  +  sread(list, C);
  v += mk;
  if (k > 0) {
    v = W_ONE + ( ((v-W_ONE) << k) | sread(list, k) );
  }
  return (v-W_ONE);
}
void  put_baer (BitList *list, int k, WTYPE value)
{
  int mk;
  WTYPE C, v, postword;

  assert(k >= -32);
  assert(k <= 32);
  mk = (k < 0) ? -k : 0;

  if (value < mk) {
    put_unary1(list, value);
    return;
  }
  v = (k == 0)  ?  value+W_ONE
                :  (k < 0)  ?  value-mk+W_ONE  :  W_ONE + (value >> k);
  C = 0;
  postword = 0;

  /* This ensures ~0 is encoded correctly. */
  if ( (k == 0) & (value >= 3) ) {
    if ((value & 1) == 0) { v = (value - W_CONST(2)) >> 1;  postword = W_ONE; }
    else                  { v = (value - W_ONE) >> 1; }
    C = 1;
  }

  while (v >= 4) {
    if ((v & 1) == 0) { v = (v - W_CONST(2)) >> 1; }
    else              { v = (v - W_CONST(3)) >> 1; postword |= (W_ONE << C); }
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
  int    s [BITS_PER_WORD / 2];    /* shift amount */
  WTYPE  t [BITS_PER_WORD / 2];    /* threshold    */
} bvzeta_map;

bvzeta_map bvzeta_map_cache[16] = {0};

static void bv_make_param_map(int k)
{
  bvzeta_map* bvm;
  assert(k >= 2);
  assert(k <= 15);
  bvm = &(bvzeta_map_cache[k]);
  if (bvm->maxhk == 0) {
    int h;
    int maxh = (BITS_PER_WORD - 1) / k;
    assert(maxh < (BITS_PER_WORD/2));
    for (h = 0; h <= maxh; h++) {
      int hk = h * k;
      int s = 1;
      WTYPE interval, z;
      interval = (W_ONE << (hk+k)) - (W_ONE << hk) - W_ONE;
      z = interval + W_ONE;
      { WTYPE v = z; while ( (v >>= 1) != 0)  s++; } /* ceil log2(z) */
      assert(s >= 2);
      bvm->s[h] = s;
      bvm->t[h] = (W_ONE << s) - z;  /* threshold */
    }
    bvm->maxhk = maxh * k;
  }
}

WTYPE get_boldivigna (BitList *list, int k)
{
  bvzeta_map* bvm;
  int maxh, s;
  WTYPE h, t, first, v;

  assert(k >= 1);
  assert(k <= 15);  /* You should use Delta codes for anything over 6. */

  if (k == 1)  return get_gamma(list);

  bvm = &(bvzeta_map_cache[k]);
  if (bvm->maxhk == 0)
    bv_make_param_map(k);

  maxh = bvm->maxhk / k;
  assert(maxh < (BITS_PER_WORD/2));

  h = get_unary(list);
  if (h > maxh) return W_FFFF;
  s = bvm->s[h];
  t = bvm->t[h];
  assert(s >= 2);
  first = sread(list, s-1);
  if (first >= t)
    first = (first << 1) + sread(list, 1) - t;

  v = (W_ONE << (h*k)) - W_ONE + first;   /* -1 is to make 0 based */
  return v;
}
void  put_boldivigna (BitList *list, int k, WTYPE value)
{
  bvzeta_map* bvm;
  int maxh, maxhk, hk, h, s;
  WTYPE t, x;

  assert(k >= 1);
  assert(k <= 15);  /* You should use Delta codes for anything over 6. */

  if (k == 1)  { put_gamma(list, value); return; }

  bvm = &(bvzeta_map_cache[k]);
  if (bvm->maxhk == 0)
    bv_make_param_map(k);

  maxh = bvm->maxhk / k;
  assert(maxh < (BITS_PER_WORD/2));

  if (value == W_FFFF)  { put_unary(list, maxh+1); return; }

  maxhk = maxh * k;
  hk = 0;
  while ( (hk < maxhk) && (value >= ((W_ONE << (hk+k))-W_ONE)) )  hk += k;
  h = hk/k;
  assert(h <= maxh);
  s = bvm->s[h];
  t = bvm->t[h];

  put_unary(list, h);
  x = value - (W_ONE << hk) + W_ONE;
  if (x < t)
    swrite(list, s-1, x);
  else
    swrite(list, s, x+t);
}

WTYPE get_rice_sub (BitList *list, SV* self, SV* code, int k)
{
  WTYPE v;
  assert( (k >= 0) && (k <= BITS_PER_WORD) );
  assert( ((code == 0) && (self == 0))  ||  ((code != 0) && (self != 0)) );

  v = (code == 0)  ?  get_unary(list)  :  call_get_sub(self, code, list);
  if (k > 0)
    v = (v << k) | sread(list, k);
  return v;
}
void  put_rice_sub (BitList *list, SV* self, SV* code, int k, WTYPE value)
{
  WTYPE q;
  assert( (k >= 0) && (k <= BITS_PER_WORD) );
  assert( ((code == 0) && (self == 0))  ||  ((code != 0) && (self != 0)) );

  q = value >> k;
  if (code == 0) { put_unary(list, q); }
  else           { call_put_sub(self, code, list, q); }
  if (k > 0) {
    WTYPE r = value - (q << k);
    swrite(list, k, r);
  }
}

WTYPE get_gamma_rice (BitList *list, int k)
{
  WTYPE v;
  assert(k >= 0);
  assert(k <= BITS_PER_WORD);
  v = get_gamma(list);
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

WTYPE get_golomb_sub (BitList *list, SV* self, SV* code, WTYPE m)
{
  int base = 1;
  WTYPE threshold, q, v;

  assert(m >= W_ONE);
  assert( ((code == 0) && (self == 0))  ||  ((code != 0) && (self != 0)) );

  q = (code == 0)  ?  get_unary(list)  :  call_get_sub(self, code, list);
  if (m == W_ONE)  return q;

  base = 1;
  {
    WTYPE v = m-W_ONE;
    while (v >>= 1)  base++;
  }
  threshold = (W_ONE << base) - m;

  v = q * m;
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
void  put_golomb_sub (BitList *list, SV* self, SV* code, WTYPE m, WTYPE value)
{
  int base = 1;
  WTYPE threshold, q, r;

  assert(m >= W_ONE);
  assert( ((code == 0) && (self == 0))  ||  ((code != 0) && (self != 0)) );

  if (m == W_ONE) {
    if (code == 0) { put_unary(list, value); }
    else           { call_put_sub(self, code, list, value); }
    return;
  }

  {
    WTYPE v = m-W_ONE;
    while (v >>= 1)  base++;
  }
  threshold = (W_ONE << base) - m;

  q = value / m;
  r = value - (q * m);
  assert(r >= 0);
  assert(r < m);
  assert(q*m+r == value);
  if (code == 0) { put_unary(list, q); }
  else           { call_put_sub(self, code, list, q); }
  if (r < threshold)
    swrite(list, base-1, r);
  else
    swrite(list, base, r + threshold);
}

WTYPE get_gamma_golomb (BitList *list, WTYPE m)
{
  int base = 1;
  WTYPE threshold, q, v;

  assert(m >= W_ONE);

  q = get_gamma(list);
  if (m == W_ONE)  return q;

  {
    WTYPE v = m-W_ONE;
    while (v >>= 1)  base++;
  }
  threshold = (W_ONE << base) - m;

  v = q * m;
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
  int base = 1;
  WTYPE threshold, q, r;

  assert(m >= W_ONE);
  if (m == W_ONE) {
    put_gamma(list, value);
    return;
  }

  {
    WTYPE v = m-W_ONE;
    while (v >>= 1)  base++;
  }
  threshold = (W_ONE << base) - m;

  q = value / m;
  r = value - (q * m);
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
WTYPE get_adaptive_gamma_rice_sub (BitList *list, SV* self, SV* code, int *kp)
{
  int k;
  WTYPE q, v;

  assert( ((code == 0) && (self == 0))  ||  ((code != 0) && (self != 0)) );
  assert( (list != 0) && (kp != 0) );
  k = *kp;
  assert( (k >= 0) && (k <= BITS_PER_WORD) );

  q = (code == 0)  ?  get_gamma(list)  :  call_get_sub(self, code, list);
  v = q << k;
  if (k > 0)
    v |= sread(list, k);
  if ( (q <= QLOW ) && (k > 0            ) )  *kp -= 1;
  if ( (q >= QHIGH) && (k < BITS_PER_WORD) )  *kp += 1;
  return v;
}
void  put_adaptive_gamma_rice_sub (BitList *list, SV* self, SV* code, int *kp, WTYPE value)
{
  int k;
  WTYPE q;

  assert( ((code == 0) && (self == 0))  ||  ((code != 0) && (self != 0)) );
  assert( (list != 0) && (kp != 0) );
  k = *kp;
  assert( (k >= 0) && (k <= BITS_PER_WORD) );

  q = value >> k;
  if (code == 0) { put_gamma(list, q); }
  else           { call_put_sub(self, code, list, q); }

  if (k > 0) {
    WTYPE r = value - (q << k);
    swrite(list, k, r);
  }
  if ( (q <= QLOW ) && (k > 0            ) )  *kp -= 1;
  if ( (q >= QHIGH) && (k < BITS_PER_WORD) )  *kp += 1;
}


typedef struct {
  int    size;       /* only defined in first entry */
  int    prefix;
  int    bits;
  WTYPE  prefix_cmp;
  WTYPE  minval;
  WTYPE  maxval;
} startstop_map_entry;

char* make_startstop_prefix_map(SV* paramref)
{
  int nparams, prefix_size, prefix, bits, p;
  WTYPE prefix_cmp, minval, maxval;
  startstop_map_entry* map;

  assert(paramref != 0);
  if (    (!SvROK(paramref))
       || (SvTYPE(SvRV(paramref)) != SVt_PVAV)
       || ((nparams = av_len((AV *)SvRV(paramref))+1) < 2)) {
    croak("invalid parameters: startstop ref");
    return 0;
  }

  map = (startstop_map_entry*) malloc(nparams * sizeof(startstop_map_entry));
  if (map == 0) {
    croak("allocation failure");
    return 0;
  }

  prefix_size = nparams-1;
  prefix_cmp = W_ONE << prefix_size;
  prefix = 0;
  bits = 0;
  minval = 0;
  maxval = 0;

  for (p = 0; p < nparams; p++) {
    int step;
    SV** step_sv = av_fetch((AV *)SvRV(paramref), p, 0);
    if ( (step_sv == 0) || (SvIV(*step_sv) < 0) ) {
      croak("invalid parameters: startstop step");
      free(map);
      return 0;
    }
    step = (*step_sv != &PL_sv_undef)  ?  SvIV(*step_sv)  :  BITS_PER_WORD;
    bits += (*step_sv != &PL_sv_undef)  ?  SvIV(*step_sv)  :  BITS_PER_WORD;
    if (bits > BITS_PER_WORD)  bits = BITS_PER_WORD;
    if (p == 0)
      minval = 0;
    else
      minval += maxval+1;
    maxval = (bits < BITS_PER_WORD)  ?  (W_ONE << bits)-W_ONE  :  W_FFFF;
    prefix++;
    prefix_cmp >>= 1;
    map[p].prefix = prefix;
    map[p].bits = bits;
    map[p].prefix_cmp = prefix_cmp;
    map[p].minval = minval;
    map[p].maxval = ((minval+maxval)<maxval) ? W_FFFF : minval+maxval;

  }
  map[0].size = nparams;
  /* Patch last value */
  map[nparams-1].prefix--;

  return (char*) map;
}

WTYPE get_startstop  (BitList *list, char* cmap)
{
  int nparams, prefix, prefix_bits, bits;
  WTYPE minval;
  WTYPE v;
  WTYPE look;
  int looksize;
  startstop_map_entry* map = (startstop_map_entry*) cmap;

  assert(map != 0);

  nparams = map[0].size;
  looksize = map[nparams-1].prefix;
  look = sreadahead(list, looksize);
  prefix = 0;
  while (look < map[prefix].prefix_cmp)  prefix++;
  assert(prefix < nparams);

  prefix_bits = map[prefix].prefix;
  bits        = map[prefix].bits;
  minval      = map[prefix].minval;

  list->pos += prefix_bits;
  v = minval;
  if (bits > 0)
    v += sread(list, bits);
  return v;
}

void put_startstop  (BitList *list, char* cmap, WTYPE value)
{
  int nparams, prefix, prefix_bits, bits;
  WTYPE global_maxval, prefix_cmp, minval;
  WTYPE v;
  startstop_map_entry* map = (startstop_map_entry*) cmap;

  assert(map != 0);
  nparams = map[0].size;
  global_maxval = map[nparams-1].maxval;
  if (value > map[nparams-1].maxval) {
    croak("value %lu out of range 0 - %lu", value, map[nparams-1].maxval);
    return;
  }
  prefix = 0;
  while (value > map[prefix].maxval)  prefix++;
  assert(prefix < nparams);

  prefix_bits = map[prefix].prefix;
  bits        = map[prefix].bits;
  prefix_cmp  = map[prefix].prefix_cmp;
  minval      = map[prefix].minval;

  v = value - minval;
  if ( (prefix_bits + bits) <= BITS_PER_WORD ) {
    if (prefix_cmp != 0)
      v |= W_ONE << bits;
    swrite(list, prefix_bits + bits, v);
  } else {
    if (prefix_cmp == 0)
      swrite(list, prefix_bits, 0);
    else
      put_unary(list, prefix_bits-1);
    if (bits > 0)
      swrite(list, bits, v);
  }
}
