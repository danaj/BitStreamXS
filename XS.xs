
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

/* We're not using anything for which we need ppport.h */

#include "bitlist.h"

#define CHECKPOS \
  if (list->pos >= list->len) \
      XSRETURN_UNDEF;

#ifdef INT_MAX
  #define MAX_COUNT INT_MAX
#else
  #define MAX_COUNT 1000000000 /* big enough */
#endif

/*
 * Instead of using XPUSH to extend the stack one value at a time, we'll
 * extend this many when we run out.
 */
#define BLSTGROW 64

/* Doing this cleanly without varargs would be frustrating.
 * It means strict C89 compilers won't work.
 * I should probably replace it at some point.
 */
#define GET_CODEVP(codename, nargs, ...) \
   { \
    bool wantarray = (GIMME_V == G_ARRAY); \
    unsigned long v; \
    int c = 0; \
    if ( (list == 0) || (count == 0) || (list->pos >= list->len) ) { \
      if (wantarray) { XSRETURN_EMPTY; } else { XSRETURN_UNDEF; } \
    } \
    if (list->is_writing) { \
      croak("read while writing with %s", #codename); \
      if (wantarray) { XSRETURN_EMPTY; } else { XSRETURN_UNDEF; } \
    } \
    if (count < 0)  count = MAX_COUNT; \
    if (!wantarray) { \
      v = 0; \
      while ( (c++ < count) && (list->pos < list->len) ) \
        v = get_ ## codename(__VA_ARGS__); \
      PUSHs(sv_2mortal(newSVuv(  v  ))); \
    } else { \
      int st_size = 0; \
      int st_pos  = 0; \
      if (count < 10000) { \
        EXTEND(SP, count); \
        st_size = count; \
      } \
      while ( (c++ < count) && (list->pos < list->len) ) { \
        if (++st_pos > st_size) { EXTEND(SP, BLSTGROW); st_size += BLSTGROW; } \
        v = get_ ## codename(__VA_ARGS__); \
        PUSHs(sv_2mortal(newSVuv(  v  ))); \
      } \
    } \
   }

#define PUT_CODEVP(codename, nargs, ...) \
    if (!list->is_writing) { \
      croak("write while reading"); \
    } else { \
      int c = (nargs); \
      while (++c < items) \
        put_ ## codename(__VA_ARGS__, SvUV(ST(c))); \
    }

#define GET_CODE(cn)          GET_CODEVP(cn, 0, list)
#define PUT_CODE(cn)          PUT_CODEVP(cn, 0, list)
#define GET_CODEP(cn,a)       GET_CODEVP(cn, 1, list, a)
#define PUT_CODEP(cn,a)       PUT_CODEVP(cn, 1, list, a)
#define GET_CODEPP(cn,a,b)    GET_CODEVP(cn, 2, list, a, b)
#define PUT_CODEPP(cn,a,b)    PUT_CODEVP(cn, 2, list, a, b)
#define GET_CODESPP(cn,s,a,b) GET_CODEVP(cn, 2, list, s, a, b)
#define PUT_CODESPP(cn,s,a,b) PUT_CODEVP(cn, 2, list, s, a, b)

typedef BitList* Data__BitStream__XS;

MODULE = Data::BitStream::XS	PACKAGE = Data::BitStream::XS

PROTOTYPES: ENABLE

Data::BitStream::XS
new (IN char* package, IN int nBits = 0)
  CODE:
    RETVAL = new(nBits);
  OUTPUT:
    RETVAL

void
DESTROY(IN Data::BitStream::XS list)

int
maxbits(IN Data::BitStream::XS list = 0)
  CODE:
    RETVAL = 8 * sizeof(WTYPE);
  OUTPUT:
    RETVAL

void
trim(IN Data::BitStream::XS list)
  CODE:
    resize(list, list->len);

int
len(IN Data::BitStream::XS list)
  CODE:
    RETVAL = list->len;
  OUTPUT:
    RETVAL

int
maxlen(IN Data::BitStream::XS list)
  CODE:
    RETVAL = list->maxlen;
  OUTPUT:
    RETVAL

int
pos(IN Data::BitStream::XS list)
  CODE:
    RETVAL = list->pos;
  OUTPUT:
    RETVAL

int
_set_pos(IN Data::BitStream::XS list, IN int n)

int
_set_len(IN Data::BitStream::XS list, IN int n)

bool
writing(IN Data::BitStream::XS list)
  CODE:
    RETVAL = list->is_writing;
  OUTPUT:
    RETVAL

void
rewind(IN Data::BitStream::XS list)
  CODE:
    if (list->is_writing)
      croak("rewind while writing");
    else
      _set_pos(list, 0);

void
skip(IN Data::BitStream::XS list, IN int bits)
  CODE:
    if (list->is_writing)
      croak("skip while writing");
    else if ((list->pos + bits) > list->len)
      croak("skip off stream");
    else
      _set_pos(list, list->pos + bits);

bool
exhausted(IN Data::BitStream::XS list)
  CODE:
    if (list->is_writing)
      croak("rewind while writing");
    RETVAL = (list->pos >= list->len);
  OUTPUT:
    RETVAL

void
erase(IN Data::BitStream::XS list)
  CODE:
    resize(list, 0);

void
read_open(IN Data::BitStream::XS list)

void
write_open(IN Data::BitStream::XS list)

void
write_close(IN Data::BitStream::XS list)

unsigned long
read(IN Data::BitStream::XS list, IN int bits, IN char* flags = 0)
  PREINIT:
    int readahead;
  CODE:
    if (list->is_writing) {
      croak("read while writing");
      XSRETURN_UNDEF;
    }
    if ( (bits <= 0) || (bits > BITS_PER_WORD) ) {
      croak("invalid bits: %d", bits);
      XSRETURN_UNDEF;
    }
    readahead = (flags != 0) && (strcmp(flags, "readahead") == 0);
    if (readahead) {
      if (list->pos >= list->len)
        XSRETURN_UNDEF;
      RETVAL = sreadahead(list, bits);
    } else {
      if ( (list->pos + bits-1) >= list->len )
        XSRETURN_UNDEF;
      RETVAL = sread(list, bits);
    }
  OUTPUT:
    RETVAL

unsigned long
readahead(IN Data::BitStream::XS list, IN int bits)
  CODE:
    if (list->is_writing) {
      croak("read while writing");
      XSRETURN_UNDEF;
    }
    if ( (bits <= 0) || (bits > BITS_PER_WORD) ) {
      croak("invalid bits: %d", bits);
      XSRETURN_UNDEF;
    }
    if (list->pos >= list->len)
      XSRETURN_UNDEF;
    RETVAL = sreadahead(list, bits);
  OUTPUT:
    RETVAL

void
write(IN Data::BitStream::XS list, IN int bits, IN unsigned long v)
  CODE:
    if (!list->is_writing) {
      croak("write while reading");
      XSRETURN_UNDEF;
    }
    if ( (bits <= 0) || (bits > BITS_PER_WORD) ) {
      croak("invalid bits: %d", bits);
      XSRETURN_UNDEF;
    }
    swrite(list, bits, v);

void
put_string(IN Data::BitStream::XS list, ...)
  CODE:
    if (!list->is_writing) {
      croak("write while reading");
    } else {
      int c = 0;
      while (++c < items)
        put_string(list, SvPV_nolen(ST(c)));
    }

SV *
read_string(IN Data::BitStream::XS list, IN int bits)
  PREINIT:
    char* buf;
  CODE:
    if (list->is_writing)
      { croak("read while writing"); XSRETURN_UNDEF; }
    if (bits < 0)
      { croak("invalid bits: %d", bits); XSRETURN_UNDEF; }
    if (bits > (list->len - list->pos))
      { croak("short read"); XSRETURN_UNDEF; }
    buf = read_string(list, bits);
    if (buf == 0) {
      XSRETURN_UNDEF;
    } else {
      RETVAL = newSVpvn(buf, bits);
      free(buf);
    }
  OUTPUT:
    RETVAL

SV*
to_raw(IN Data::BitStream::XS list)
  PREINIT:
    char* buf;
  CODE:
    buf = to_raw(list);
    if (buf == 0) {
      XSRETURN_UNDEF;
    } else {
      /* Return just the necessary number of bytes */
      size_t bytes = NBYTES(list->len);
      RETVAL = newSVpvn(buf, bytes);
      free(buf);
    }
  OUTPUT:
    RETVAL

void
from_raw(IN Data::BitStream::XS list, IN char* str, IN int bits)

void
xput_stream(IN Data::BitStream::XS list, IN Data::BitStream::XS source)
  CODE:
    if (!list->is_writing) {
      croak("write while reading");
    } else {
      xput_stream(list, source);
    }


void
get_unary(IN Data::BitStream::XS list, IN int count = 1)
  PPCODE:
    GET_CODE(unary);

void
put_unary(IN Data::BitStream::XS list, ...)
  CODE:
    PUT_CODE(unary);

void
get_unary1(IN Data::BitStream::XS list, IN int count = 1)
  PPCODE:
    GET_CODE(unary1);

void
put_unary1(IN Data::BitStream::XS list, ...)
  CODE:
    PUT_CODE(unary1);

void
get_gamma(IN Data::BitStream::XS list, IN int count = 1)
  PPCODE:
    GET_CODE(gamma);

void
put_gamma(IN Data::BitStream::XS list, ...)
  CODE:
    PUT_CODE(gamma);

void
get_delta(IN Data::BitStream::XS list, IN int count = 1)
  PPCODE:
    GET_CODE(delta);

void
put_delta(IN Data::BitStream::XS list, ...)
  CODE:
    PUT_CODE(delta);

void
get_omega(IN Data::BitStream::XS list, IN int count = 1)
  PPCODE:
    GET_CODE(omega);

void
put_omega(IN Data::BitStream::XS list, ...)
  CODE:
    PUT_CODE(omega);

void
get_fib(IN Data::BitStream::XS list, IN int count = 1)
  PPCODE:
    GET_CODE(fib);

void
put_fib(IN Data::BitStream::XS list, ...)
  CODE:
    PUT_CODE(fib);

void
get_levenstein(IN Data::BitStream::XS list, IN int count = 1)
  PPCODE:
    GET_CODE(levenstein);

void
put_levenstein(IN Data::BitStream::XS list, ...)
  CODE:
    PUT_CODE(levenstein);

void
get_evenrodeh(IN Data::BitStream::XS list, IN int count = 1)
  PPCODE:
    GET_CODE(evenrodeh);

void
put_evenrodeh(IN Data::BitStream::XS list, ...)
  CODE:
    PUT_CODE(evenrodeh);

void
get_binword(IN Data::BitStream::XS list, IN int k, IN int count = 1)
  PPCODE:
    if ( (k < 0) || (k > BITS_PER_WORD) ) {
      croak("invalid parameters: binword %d", k);
      XSRETURN_UNDEF;
    }
    GET_CODEP(binword, k);

void
put_binword(IN Data::BitStream::XS list, IN int k, ...)
  CODE:
    if ( (k < 0) || (k > BITS_PER_WORD) ) {
      croak("invalid parameters: binword %d", k);
      return;
    }
    PUT_CODEP(binword, k);

void
get_baer(IN Data::BitStream::XS list, IN int k, IN int count = 1)
  PPCODE:
    if ( (k < -32) || (k > 32) ) {
      croak("invalid parameters: baer %d", k);
      XSRETURN_UNDEF;
    }
    GET_CODEP(baer, k);

void
put_baer(IN Data::BitStream::XS list, IN int k, ...)
  CODE:
    if ( (k < -32) || (k > 32) ) {
      croak("invalid parameters: baer %d", k);
      return;
    }
    PUT_CODEP(baer, k);

void
get_boldivigna(IN Data::BitStream::XS list, IN int k, IN int count = 1)
  PPCODE:
    if ( (k < 1) || (k > 15) ) {
      croak("invalid parameters: boldivigna %d", k);
      XSRETURN_UNDEF;
    }
    GET_CODEP(boldivigna, k);

void
put_boldivigna(IN Data::BitStream::XS list, IN int k, ...)
  CODE:
    if ( (k < 1) || (k > 15) ) {
      croak("invalid parameters: boldivigna %d", k);
      return;
    }
    PUT_CODEP(boldivigna, k);

void
get_rice_sub(IN Data::BitStream::XS list, IN SV* coderef, IN int k, IN int count = 1)
  PREINIT:
    SV* self = ST(0);
    SV* cref = 0;
  PPCODE:
    if ( (k < 0) || (k > BITS_PER_WORD) ) {
      croak("invalid parameters: rice %d", k);
      XSRETURN_UNDEF;
    }
    if (!SvROK(coderef)) {
      self = 0;
      cref = 0;
    } else {
      if ((!SvROK(coderef)) || (SvTYPE(SvRV(coderef)) != SVt_PVCV) ) {
        croak("invalid parameters: rice coderef");
        return;
      }
      cref = SvRV(coderef);
    }
    GET_CODESPP(rice_sub, self, cref, k);

void
put_rice_sub(IN Data::BitStream::XS list, IN SV* coderef, IN int k, ...)
  PREINIT:
    SV* self = ST(0);
    SV* cref = 0;
  CODE:
    if ( (k < 0) || (k > BITS_PER_WORD) ) {
      croak("invalid parameters: rice %d", k);
      return;
    }
    if (!SvROK(coderef)) {
      self = 0;
      cref = 0;
    } else {
      if ((!SvROK(coderef)) || (SvTYPE(SvRV(coderef)) != SVt_PVCV) ) {
        croak("invalid parameters: rice coderef");
        return;
      }
      cref = SvRV(coderef);
    }
    PUT_CODESPP(rice_sub, self, cref, k);

void
get_gamma_rice(IN Data::BitStream::XS list, IN int k, IN int count = 1)
  ALIAS:
    get_expgolomb = 1
  PPCODE:
    if ( (k < 0) || (k > BITS_PER_WORD) ) {
      croak("invalid parameters: gamma_rice %d", k);
      XSRETURN_UNDEF;
    }
    GET_CODEP(gamma_rice, k);

void
put_gamma_rice(IN Data::BitStream::XS list, IN int k, ...)
  ALIAS:
    put_expgolomb = 1
  CODE:
    if ( (k < 0) || (k > BITS_PER_WORD) ) {
      croak("invalid parameters: gamma_rice %d", k);
      return;
    }
    PUT_CODEP(gamma_rice, k);

void
get_golomb_sub(IN Data::BitStream::XS list, IN SV* coderef, IN unsigned long m, IN int count = 1)
  PREINIT:
    SV* self = ST(0);
    SV* cref = 0;
  PPCODE:
    if (m < 1UL) {
      croak("invalid parameters: golomb %lu", m);
      XSRETURN_UNDEF;
    }
    if (!SvROK(coderef)) {
      self = 0;
      cref = 0;
    } else {
      if ((!SvROK(coderef)) || (SvTYPE(SvRV(coderef)) != SVt_PVCV) ) {
        croak("invalid parameters: golomb coderef");
        return;
      }
      cref = SvRV(coderef);
    }
    GET_CODESPP(golomb_sub, self, cref, m);

void
put_golomb_sub(IN Data::BitStream::XS list, IN SV* coderef, IN unsigned long m, ...)
  PREINIT:
    SV* self = ST(0);
    SV* cref = 0;
  CODE:
    if (m < 1UL) {
      croak("invalid parameters: golomb %lu", m);
      return;
    }
    if (!SvROK(coderef)) {
      self = 0;
      cref = 0;
    } else {
      if ((!SvROK(coderef)) || (SvTYPE(SvRV(coderef)) != SVt_PVCV) ) {
        croak("invalid parameters: golomb coderef");
        return;
      }
      cref = SvRV(coderef);
    }
    PUT_CODESPP(golomb_sub, self, cref, m);


void
get_gamma_golomb(IN Data::BitStream::XS list, IN unsigned long m, IN int count = 1)
  ALIAS:
    get_gammagolomb = 1
  PPCODE:
    if (m < 1UL) {
      croak("invalid parameters: gamma_golomb %lu", m);
      XSRETURN_UNDEF;
    }
    GET_CODEP(gamma_golomb, m);

void
put_gamma_golomb(IN Data::BitStream::XS list, IN unsigned long m, ...)
  ALIAS:
    put_gammagolomb = 1
  CODE:
    if (m < 1UL) {
      croak("invalid parameters: gamma_golomb %lu", m);
      return;
    }
    PUT_CODEP(gamma_golomb, m);

void
get_arice_sub(list, coderef, k, count=1)
      Data::BitStream::XS list
      SV* coderef
      int &k
      int count
  PREINIT:
    SV* self = ST(0);
    SV* cref = 0;
    SV* stack_k_ptr = ST(2);  /* Remember position of k, it will be modified */
  PPCODE:
    if ( (k < 0) || (k > BITS_PER_WORD) ) {
      croak("invalid parameters: adaptive_gamma_rice %d", k);
      XSRETURN_UNDEF;
    }
    if (!SvROK(coderef)) {
      self = 0;
      cref = 0;
    } else {
      if ((!SvROK(coderef)) || (SvTYPE(SvRV(coderef)) != SVt_PVCV) ) {
        croak("invalid parameters: adaptive_gamma_rice coderef");
        return;
      }
      cref = SvRV(coderef);
    }
    GET_CODESPP(adaptive_gamma_rice_sub, self, cref, &k);
    /* Return the modified k back to Perl */
    sv_setiv(stack_k_ptr, k);
    SvSETMAGIC(stack_k_ptr);

void
put_arice_sub(list, coderef, k, ...)
      Data::BitStream::XS list
      SV* coderef
      int &k
  PREINIT:
    SV* self = ST(0);
    SV* cref = 0;
  CODE:
    if ( (k < 0) || (k > BITS_PER_WORD) ) {
      croak("invalid parameters: adaptive_gamma_rice %d", k);
      return;
    }
    if (!SvROK(coderef)) {
      self = 0;
      cref = 0;
    } else {
      if ((!SvROK(coderef)) || (SvTYPE(SvRV(coderef)) != SVt_PVCV) ) {
        croak("invalid parameters: adaptive_gamma_rice coderef");
        return;
      }
      cref = SvRV(coderef);
    }
    PUT_CODESPP(adaptive_gamma_rice_sub, self, cref, &k);
  OUTPUT:
    k


void
get_startstop(IN Data::BitStream::XS list, IN SV* p, IN int count = 1)
  PREINIT:
    char* map;
  PPCODE:
    map = make_startstop_prefix_map(p);
    if (map == 0) {
      XSRETURN_UNDEF;
    }
    /* TODO: we'll skip free in some croak conditions */
    GET_CODEP(startstop, map);
    free(map);

void
put_startstop(IN Data::BitStream::XS list, IN SV* p, ...)
  PREINIT:
    char* map;
  CODE:
    map = make_startstop_prefix_map(p);
    if (map == 0)
       return;
    PUT_CODEVP(startstop, 1, list, map);
    free(map);
