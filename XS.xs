
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

#define GET_CODE(codename) \
    bool wantarray = (GIMME_V == G_ARRAY); \
    if ( (list == 0) || (count == 0) || (list->pos >= list->len) ) \
      if (wantarray) { XSRETURN_EMPTY; } else { XSRETURN_UNDEF; } \
    if (list->is_writing) { \
      croak("read while writing with %s", #codename); \
      if (wantarray) { XSRETURN_EMPTY; } else { XSRETURN_UNDEF; } \
    } \
    unsigned long v; \
    int c = 0; \
    if (count < 0)  count = MAX_COUNT; \
    if (!wantarray) { \
      while ( (c++ < count) && (list->pos < list->len) ) \
        v = get_ ## codename(list); \
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
        PUSHs(sv_2mortal(newSVuv(  get_ ## codename(list)  ))); \
      } \
    }

#define GET_CODEP(codename, param) \
    bool wantarray = (GIMME_V == G_ARRAY); \
    if ( (list == 0) || (count == 0) || (list->pos >= list->len) ) \
      if (wantarray) { XSRETURN_EMPTY; } else { XSRETURN_UNDEF; } \
    if (list->is_writing) { \
      croak("read while writing"); \
      if (wantarray) { XSRETURN_EMPTY; } else { XSRETURN_UNDEF; } \
    } \
    unsigned long v; \
    int c = 0; \
    if (count < 0)  count = MAX_COUNT; \
    if (!wantarray) { \
      while ( (c++ < count) && (list->pos < list->len) ) \
        v = get_ ## codename(list, param); \
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
        PUSHs(sv_2mortal(newSVuv(  get_ ## codename(list, param)  ))); \
      } \
    }

#define PUT_CODE(codename) \
    if (!list->is_writing) { \
      croak("write while reading"); \
    } else { \
      int c; \
      for (c = 1; c < items; c++) \
        put_ ## codename(list, SvUV(ST(c))); \
    }

#define PUT_CODEP(codename, param) \
    if (!list->is_writing) { \
      croak("write while reading"); \
    } else { \
      int c; \
      for (c = 2; c < items; c++) \
        put_ ## codename(list, param, SvUV(ST(c))); \
    }

typedef BitList* Data__BitStream__XS;

MODULE = Data::BitStream::XS	PACKAGE = Data::BitStream::XS

PROTOTYPES: ENABLE

Data::BitStream::XS
new(package, nBits = 0)
	char *package
	int   nBits
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
      set_pos(list, 0);

void
skip(IN Data::BitStream::XS list, IN int bits)
  CODE:
    if (list->is_writing)
      croak("skip while writing");
    else if ((list->pos + bits) > list->len)
      croak("skip off stream");
    else
      set_pos(list, list->pos + bits);

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
read(IN Data::BitStream::XS list, IN int bits)
  CODE:
    if (list->is_writing) {
      croak("read while writing");
      XSRETURN_UNDEF;
    }
    if ( (bits <= 0) || (bits > BITS_PER_WORD) ) {
      croak("invalid bits: %d", bits);
      XSRETURN_UNDEF;
    }
    int pos = list->pos;
    int len = list->len;
    if ( (pos >= len) || ((pos+bits) > len) )
      XSRETURN_UNDEF;
    RETVAL = sread(list, bits);
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
put_string(list, s)
	Data::BitStream::XS list
	char* s

SV *
read_string(list, bits)
	Data::BitStream::XS list
	int bits
  CODE:
    char* buf = read_string(list, bits);
    if (buf == 0) {
      XSRETURN_UNDEF;
    } else {
      RETVAL = newSVpvn(buf, bits);
      free(buf);
    }
  OUTPUT:
    RETVAL

SV*
to_raw(list)
	Data::BitStream::XS list
  CODE:
    char* buf = to_raw(list);
    if (buf == 0) {
      XSRETURN_UNDEF;
    } else {
      //size_t bpw = 8 * sizeof(WTYPE);
      //size_t words = (list->len + (bpw-1)) / bpw;
      //size_t bytes = words * sizeof(WTYPE);
      size_t bytes = NBYTES(list->len);
      RETVAL = newSVpvn(buf, bytes);
      free(buf);
    }
  OUTPUT:
    RETVAL

void
from_raw(list, str, bits)
	Data::BitStream::XS list
	char* str
	int bits



void
get_unary(list, count = 1) 
        Data::BitStream::XS list
	int count
  PPCODE:
    GET_CODE(unary);

void
put_unary(list, ...)
	Data::BitStream::XS list
  CODE:
    PUT_CODE(unary);

void
get_unary1(list, count = 1)
	Data::BitStream::XS list
	int count
  PPCODE:
    GET_CODE(unary1);

void
put_unary1(list, ...)
	Data::BitStream::XS list
  CODE:
    PUT_CODE(unary1);

void
get_gamma(list, count = 1)
	Data::BitStream::XS list
	int count
  PPCODE:
    GET_CODE(gamma);

void
put_gamma(list, ...)
	Data::BitStream::XS list
  CODE:
    PUT_CODE(gamma);

void
get_delta(list, count = 1)
	Data::BitStream::XS list
	int count
  PPCODE:
    GET_CODE(delta);

void
put_delta(list, ...)
	Data::BitStream::XS list
  CODE:
    PUT_CODE(delta);

void
get_omega(list, count = 1)
	Data::BitStream::XS list
	int count
  PPCODE:
    GET_CODE(omega);

void
put_omega(list, ...)
	Data::BitStream::XS list
  CODE:
    PUT_CODE(omega);

void
get_fib(list, count = 1)
	Data::BitStream::XS list
	int count
  PPCODE:
    GET_CODE(fib);

void
put_fib(list, ...)
	Data::BitStream::XS list
  CODE:
    PUT_CODE(fib);

void
get_levenstein(list, count = 1)
	Data::BitStream::XS list
	int count
  PPCODE:
    GET_CODE(levenstein);

void
put_levenstein(list, ...)
	Data::BitStream::XS list
  CODE:
    PUT_CODE(levenstein);

void
get_evenrodeh(list, count = 1)
	Data::BitStream::XS list
	int count
  PPCODE:
    GET_CODE(evenrodeh);

void
put_evenrodeh(list, ...)
	Data::BitStream::XS list
  CODE:
    PUT_CODE(evenrodeh);

void
get_binword(list, k, count = 1)
	Data::BitStream::XS list
        int k
	int count
  PPCODE:
    if ( (k < 0) || (k > BITS_PER_WORD) ) {
      croak("invalid parameters: binword %d", k);
      XSRETURN_UNDEF;
    }
    GET_CODEP(binword, k);

void
put_binword(list, k, ...)
	Data::BitStream::XS list
	int k
  CODE:
    if ( (k < 0) || (k > BITS_PER_WORD) ) {
      croak("invalid parameters: binword %d", k);
      return;
    }
    PUT_CODEP(binword, k);

void
get_baer(list, k, count = 1)
	Data::BitStream::XS list
        int k
	int count
  PPCODE:
    if ( (k < -32) || (k > 32) ) {
      croak("invalid parameters: baer %d", k);
      XSRETURN_UNDEF;
    }
    GET_CODEP(baer, k);

void
put_baer(list, k, ...)
	Data::BitStream::XS list
	int k
  CODE:
    if ( (k < -32) || (k > 32) ) {
      croak("invalid parameters: baer %d", k);
      return;
    }
    PUT_CODEP(baer, k);

void
get_rice(list, k, count = 1)
	Data::BitStream::XS list
        int k
	int count
  PPCODE:
    if ( (k < 0) || (k > BITS_PER_WORD) ) {
      croak("invalid parameters: rice %d", k);
      XSRETURN_UNDEF;
    }
    GET_CODEP(rice, k);

void
put_rice(list, k, ...)
	Data::BitStream::XS list
	int k
  CODE:
    if ( (k < 0) || (k > BITS_PER_WORD) ) {
      croak("invalid parameters: rice %d", k);
      return;
    }
    PUT_CODEP(rice, k);

void
get_gamma_rice(list, k, count = 1)
	Data::BitStream::XS list
        int k
	int count
  ALIAS:
    get_expgolomb = 1
  PPCODE:
    if ( (k < 0) || (k > BITS_PER_WORD) ) {
      croak("invalid parameters: gamma_rice %d", k);
      XSRETURN_UNDEF;
    }
    GET_CODEP(gamma_rice, k);

void
put_gamma_rice(list, k, ...)
	Data::BitStream::XS list
	int k
  ALIAS:
    put_expgolomb = 1
  CODE:
    if ( (k < 0) || (k > BITS_PER_WORD) ) {
      croak("invalid parameters: gamma_rice %d", k);
      return;
    }
    PUT_CODEP(gamma_rice, k);

void
get_golomb(list, m, count = 1)
	Data::BitStream::XS list
        unsigned long m
	int count
  PPCODE:
    if (m < 1UL) {
      croak("invalid parameters: golomb %d", m);
      XSRETURN_UNDEF;
    }
    GET_CODEP(golomb, m);

void
put_golomb(list, m, ...)
	Data::BitStream::XS list
	unsigned long m
  CODE:
    if (m < 1UL) {
      croak("invalid parameters: golomb %d", m);
      return;
    }
    PUT_CODEP(golomb, m);

void
get_gamma_golomb(list, m, count = 1)
	Data::BitStream::XS list
        int m
	int count
  ALIAS:
    get_gammagolomb = 1
  PPCODE:
    if (m < 1UL) {
      croak("invalid parameters: gamma_golomb %d", m);
      XSRETURN_UNDEF;
    }
    GET_CODEP(gamma_golomb, m);

void
put_gamma_golomb(list, m, ...)
	Data::BitStream::XS list
	int m
  ALIAS:
    put_gammagolomb = 1
  CODE:
    if (m < 1UL) {
      croak("invalid parameters: gamma_golomb %d", m);
      return;
    }
    PUT_CODEP(gamma_golomb, m);

void
get_adaptive_gamma_rice(list, k, count=1)
      Data::BitStream::XS list
      int &k
      int count
  PREINIT:
    /* Note the stack position of k, so we can modify it */
    SV* stack_k_ptr = ST(1);
  PPCODE:
    if ( (k < 0) || (k > BITS_PER_WORD) ) {
      croak("invalid parameters: adaptive_gamma_rice %d", k);
      XSRETURN_UNDEF;
    }
    GET_CODEP(adaptive_gamma_rice, &k);
    /* Return the modified k back to Perl */
    sv_setiv(stack_k_ptr, k);
    SvSETMAGIC(stack_k_ptr);

void
put_adaptive_gamma_rice(list, k, ...)
	Data::BitStream::XS list
	int &k
  CODE:
    if ( (k < 0) || (k > BITS_PER_WORD) ) {
      croak("invalid parameters: adaptive_gamma_rice %d", k);
      return;
    }
    PUT_CODEP(adaptive_gamma_rice, &k);
  OUTPUT:
    k

void
get_startstepstop(IN Data::BitStream::XS list, IN SV* p, IN int count = 1)
  CODE:
    croak("not implemented");

void
put_startstepstop(IN Data::BitStream::XS list, IN SV* p, ...)
  CODE:
    croak("not implemented");

void
get_startstop(IN Data::BitStream::XS list, IN SV* p, IN int count = 1)
  CODE:
    croak("not implemented");

void
put_startstop(IN Data::BitStream::XS list, IN SV* p, ...)
  CODE:
    croak("not implemented");

void
get_boldivigna(IN Data::BitStream::XS list, IN int k, IN int count = 1)
  CODE:
    croak("not implemented");

void
put_boldivigna(IN Data::BitStream::XS list, IN int k, ...)
  CODE:
    croak("not implemented");
