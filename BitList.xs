
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

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
      wantarray  ?  XSRETURN_EMPTY  :  XSRETURN_UNDEF; \
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
      wantarray  ?  XSRETURN_EMPTY  :  XSRETURN_UNDEF; \
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

typedef BitList *Data__BitStream__BitList;

MODULE = Data::BitStream::BitList	PACKAGE = Data::BitStream::BitList

PROTOTYPES: ENABLE

Data::BitStream::BitList
new(package, nBits = 0)
	char *package
	int   nBits
	CODE:
	RETVAL = new(nBits);
	OUTPUT:
	RETVAL

void 
DESTROY(IN Data::BitStream::BitList list)

int
maxbits(IN Data::BitStream::BitList list = 0)
  CODE:
    RETVAL = 8 * sizeof(WTYPE);
  OUTPUT:
    RETVAL

void
trim(IN Data::BitStream::BitList list)
  CODE:
    resize(list, list->len);

int
len(IN Data::BitStream::BitList list)
  CODE:
    RETVAL = list->len;
  OUTPUT:
    RETVAL

int
maxlen(IN Data::BitStream::BitList list)
  CODE:
    RETVAL = list->maxlen;
  OUTPUT:
    RETVAL

int
pos(IN Data::BitStream::BitList list)
  CODE:
    RETVAL = list->pos;
  OUTPUT:
    RETVAL

int
setpos(IN Data::BitStream::BitList list, IN int n)

int
setlen(IN Data::BitStream::BitList list, IN int n)

bool
writing(IN Data::BitStream::BitList list)
  CODE:
    RETVAL = list->is_writing;
  OUTPUT:
    RETVAL

void
rewind(IN Data::BitStream::BitList list)
  CODE:
    if (list->is_writing)
      croak("rewind while writing");
    else
      setpos(list, 0);

void
skip(IN Data::BitStream::BitList list, IN int bits)
  CODE:
    if (list->is_writing)
      croak("skip while writing");
    else if ((list->pos + bits) > list->len)
      croak("skip off stream");
    else
      setpos(list, list->pos + bits);

bool
exhausted(IN Data::BitStream::BitList list)
  CODE:
    if (list->is_writing)
      croak("rewind while writing");
    RETVAL = (list->pos >= list->len);
  OUTPUT:
    RETVAL

void
erase(IN Data::BitStream::BitList list)
  CODE:
    resize(list, 0);

void
read_open(IN Data::BitStream::BitList list)

void
write_open(IN Data::BitStream::BitList list)

void
write_close(IN Data::BitStream::BitList list)

unsigned long
read(IN Data::BitStream::BitList list, IN int bits)
  CODE:
    int pos = list->pos;
    int len = list->len;
    if ( (pos >= len) || ((pos+bits) > len) )
      XSRETURN_UNDEF;
    else
      RETVAL = vread(list, bits);
  OUTPUT:
    RETVAL

unsigned long
readahead(IN Data::BitStream::BitList list, IN int bits)
  CODE:
    CHECKPOS;
    RETVAL = vreadahead(list, bits);
  OUTPUT:
    RETVAL

void
write(IN Data::BitStream::BitList list, IN int bits, IN unsigned long v)
  CODE:
    vwrite(list, bits, v);

void
put_string(list, s)
	Data::BitStream::BitList list
	char* s

SV *
read_string(list, bits)
	Data::BitStream::BitList list
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
	Data::BitStream::BitList list
  CODE:
    char* buf = to_raw(list);
    if (buf == 0) {
      XSRETURN_UNDEF;
    } else {
      size_t bpw = 8 * sizeof(WTYPE);
      size_t words = (list->len + (bpw-1)) / bpw;
      size_t bytes = words * sizeof(WTYPE);
      RETVAL = newSVpvn(buf, bytes);
      free(buf);
    }
  OUTPUT:
    RETVAL

void
from_raw(list, str, bits)
	Data::BitStream::BitList list
	char* str
	int bits



void
get_unary(list, count = 1) 
        Data::BitStream::BitList list
	int count
  PPCODE:
    GET_CODE(unary);

void
put_unary(list, ...)
	Data::BitStream::BitList list
  CODE:
    int c;
    for (c = 1; c < items; c++)
      put_unary(list, SvUV(ST(c)));

void
get_unary1(list, count = 1)
	Data::BitStream::BitList list
	int count
  PPCODE:
    GET_CODE(unary1);

void
put_unary1(list, ...)
	Data::BitStream::BitList list
  CODE:
    int c;
    for (c = 1; c < items; c++)
      put_unary1(list, SvUV(ST(c)));

void
get_gamma(list, count = 1)
	Data::BitStream::BitList list
	int count
  PPCODE:
    GET_CODE(gamma);

void
put_gamma(list, ...)
	Data::BitStream::BitList list
  CODE:
    int c;
    for (c = 1; c < items; c++)
      put_gamma(list, SvUV(ST(c)));

void
get_delta(list, count = 1)
	Data::BitStream::BitList list
	int count
  PPCODE:
    GET_CODE(delta);

void
put_delta(list, ...)
	Data::BitStream::BitList list
  CODE:
    int c;
    for (c = 1; c < items; c++)
      put_delta(list, SvUV(ST(c)));

void
get_omega(list, count = 1)
	Data::BitStream::BitList list
	int count
  PPCODE:
    GET_CODE(omega);

void
put_omega(list, ...)
	Data::BitStream::BitList list
  CODE:
    int c;
    for (c = 1; c < items; c++)
      put_omega(list, SvUV(ST(c)));

void
get_fib(list, count = 1)
	Data::BitStream::BitList list
	int count
  PPCODE:
    GET_CODE(fib);

void
put_fib(list, ...)
	Data::BitStream::BitList list
  CODE:
    int c;
    for (c = 1; c < items; c++)
      put_fib(list, SvUV(ST(c)));

void
get_levenstein(list, count = 1)
	Data::BitStream::BitList list
	int count
  PPCODE:
    GET_CODE(levenstein);

void
put_levenstein(list, ...)
	Data::BitStream::BitList list
  CODE:
    int c;
    for (c = 1; c < items; c++)
      put_levenstein(list, SvUV(ST(c)));

void
get_evenrodeh(list, count = 1)
	Data::BitStream::BitList list
	int count
  PPCODE:
    GET_CODE(evenrodeh);

void
put_evenrodeh(list, ...)
	Data::BitStream::BitList list
  CODE:
    int c;
    for (c = 1; c < items; c++)
      put_evenrodeh(list, SvUV(ST(c)));

void
get_binword(list, k, count = 1)
	Data::BitStream::BitList list
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
	Data::BitStream::BitList list
	int k
  CODE:
    if ( (k < 0) || (k > BITS_PER_WORD) ) {
      croak("invalid parameters: binword %d", k);
    } else {
      int c;
      for (c = 2; c < items; c++)
        put_binword(list, k, SvUV(ST(c)));
    }

void
get_baer(list, k, count = 1)
	Data::BitStream::BitList list
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
	Data::BitStream::BitList list
	int k
  CODE:
    if ( (k < -32) || (k > 32) ) {
      croak("invalid parameters: baer %d", k);
    } else {
      int c;
      for (c = 2; c < items; c++)
        put_baer(list, k, SvUV(ST(c)));
    }

void
get_rice(list, k, count = 1)
	Data::BitStream::BitList list
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
	Data::BitStream::BitList list
	int k
  CODE:
    if ( (k < 0) || (k > BITS_PER_WORD) ) {
      croak("invalid parameters: rice %d", k);
    } else {
      int c;
      for (c = 2; c < items; c++)
        put_rice(list, k, SvUV(ST(c)));
    }

void
get_gamma_rice(list, k, count = 1)
	Data::BitStream::BitList list
        int k
	int count
  PPCODE:
    if ( (k < 0) || (k > BITS_PER_WORD) ) {
      croak("invalid parameters: gamma_rice %d", k);
      XSRETURN_UNDEF;
    }
    GET_CODEP(gamma_rice, k);

void
put_gamma_rice(list, k, ...)
	Data::BitStream::BitList list
	int k
  CODE:
    if ( (k < 0) || (k > BITS_PER_WORD) ) {
      croak("invalid parameters: gamma_rice %d", k);
    } else {
      int c;
      for (c = 2; c < items; c++)
        put_gamma_rice(list, k, SvUV(ST(c)));
    }

void
get_golomb(list, m, count = 1)
	Data::BitStream::BitList list
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
	Data::BitStream::BitList list
	unsigned long m
  CODE:
    if (m < 1UL) {
      croak("invalid parameters: golomb %d", m);
    } else {
      int c;
      for (c = 2; c < items; c++)
        put_golomb(list, m, SvUV(ST(c)));
    }

void
get_gamma_golomb(list, m, count = 1)
	Data::BitStream::BitList list
        int m
	int count
  PPCODE:
    if (m < 1UL) {
      croak("invalid parameters: gamma_golomb %d", m);
      XSRETURN_UNDEF;
    }
    GET_CODEP(gamma_golomb, m);

void
put_gamma_golomb(list, m, ...)
	Data::BitStream::BitList list
	int m
  CODE:
    if (m < 1UL) {
      croak("invalid parameters: gamma_golomb %d", m);
    } else {
      int c;
      for (c = 2; c < items; c++)
        put_gamma_golomb(list, m, SvUV(ST(c)));
    }

void
get_adaptive_gamma_rice(list, k, count=1)
      Data::BitStream::BitList list
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
	Data::BitStream::BitList list
	int &k
  CODE:
    if ( (k < 0) || (k > BITS_PER_WORD) ) {
      croak("invalid parameters: adaptive_gamma_rice %d", k);
    } else {
      int c;
      for (c = 2; c < items; c++)
        put_adaptive_gamma_rice(list, &k, SvUV(ST(c)));
    }
  OUTPUT:
    k
