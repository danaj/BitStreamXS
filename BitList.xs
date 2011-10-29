
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "bitlist.h"

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
DESTROY(list)
	Data::BitStream::BitList list

void 
dump(list)
	Data::BitStream::BitList list

void
resize(list, n)
	Data::BitStream::BitList list
	int	 n

int
getlen(list)
	Data::BitStream::BitList list

int
getmaxlen(list)
	Data::BitStream::BitList list

int
getpos(list)
	Data::BitStream::BitList list

int
setpos(list, n)
	Data::BitStream::BitList list
	int	 n

int
setlen(list, n)
	Data::BitStream::BitList list
	int	 n



unsigned long
vread(list, bits)
	Data::BitStream::BitList list
	int bits
  CODE:
    int pos = getpos(list);
    int len = getlen(list);
    if ( (pos >= len) || ((pos+bits) > len) )
      XSRETURN_UNDEF;
    else
      RETVAL = vread(list, bits);
  OUTPUT:
    RETVAL

unsigned long
vreadahead(list, bits)
	Data::BitStream::BitList list
	int bits
  CODE:
    if (getpos(list) >= getlen(list))
      XSRETURN_UNDEF;
    else
      RETVAL = vreadahead(list, bits);
  OUTPUT:
    RETVAL

void
vwrite(list, bits, v)
	Data::BitStream::BitList list
	int bits
	unsigned long v

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
      size_t words = (getlen(list) + (bpw-1)) / bpw;
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




unsigned long
get_unary(list)
	Data::BitStream::BitList list
  CODE:
    if (getpos(list) >= getlen(list))
      XSRETURN_UNDEF;
    else
      RETVAL = get_unary(list);
  OUTPUT:
    RETVAL

void
put_unary(list, v)
	Data::BitStream::BitList list
	unsigned long	 v

unsigned long
get_unary1(list)
	Data::BitStream::BitList list
  CODE:
    if (getpos(list) >= getlen(list))
      XSRETURN_UNDEF;
    else
      RETVAL = get_unary1(list);
  OUTPUT:
    RETVAL

void
put_unary1(list, v)
	Data::BitStream::BitList list
	unsigned long	 v

unsigned long
get_gamma(list)
	Data::BitStream::BitList list
  CODE:
    if (getpos(list) >= getlen(list))
      XSRETURN_UNDEF;
    else
      RETVAL = get_gamma(list);
  OUTPUT:
    RETVAL

void
put_gamma(list, v)
	Data::BitStream::BitList list
	unsigned long	 v

unsigned long
get_delta(list)
	Data::BitStream::BitList list
  CODE:
    if (getpos(list) >= getlen(list))
      XSRETURN_UNDEF;
    else
      RETVAL = get_delta(list);
  OUTPUT:
    RETVAL

void
put_delta(list, v)
	Data::BitStream::BitList list
	unsigned long	 v

unsigned long
get_omega(list)
	Data::BitStream::BitList list
  CODE:
    if (getpos(list) >= getlen(list))
      XSRETURN_UNDEF;
    else
      RETVAL = get_omega(list);
  OUTPUT:
    RETVAL

void
put_omega(list, v)
	Data::BitStream::BitList list
	unsigned long	 v

unsigned long
get_fib(list)
	Data::BitStream::BitList list
  CODE:
    if (getpos(list) >= getlen(list))
      XSRETURN_UNDEF;
    else
      RETVAL = get_fib(list);
  OUTPUT:
    RETVAL

void
put_fib(list, v)
	Data::BitStream::BitList list
	unsigned long	 v

unsigned long
get_levenstein(list)
	Data::BitStream::BitList list
  CODE:
    if (getpos(list) >= getlen(list))
      XSRETURN_UNDEF;
    else
      RETVAL = get_levenstein(list);
  OUTPUT:
    RETVAL

void
put_levenstein(list, v)
	Data::BitStream::BitList list
	unsigned long	 v

unsigned long
get_evenrodeh(list)
	Data::BitStream::BitList list
  CODE:
    if (getpos(list) >= getlen(list))
      XSRETURN_UNDEF;
    else
      RETVAL = get_evenrodeh(list);
  OUTPUT:
    RETVAL

void
put_evenrodeh(list, v)
	Data::BitStream::BitList list
	unsigned long	 v

unsigned long
get_baer(list, k)
	Data::BitStream::BitList list
        int k
  CODE:
    if (getpos(list) >= getlen(list))
      XSRETURN_UNDEF;
    else
      RETVAL = get_baer(list, k);
  OUTPUT:
    RETVAL

void
put_baer(list, k, v)
	Data::BitStream::BitList list
	int k
	unsigned long v

unsigned long
get_rice(list, k)
	Data::BitStream::BitList list
        int k
  CODE:
    if (getpos(list) >= getlen(list))
      XSRETURN_UNDEF;
    else
      RETVAL = get_rice(list, k);
  OUTPUT:
    RETVAL

void
put_rice(list, k, v)
	Data::BitStream::BitList list
	int k
	unsigned long v

unsigned long
get_gamma_rice(list, k)
	Data::BitStream::BitList list
        int k
  CODE:
    if (getpos(list) >= getlen(list))
      XSRETURN_UNDEF;
    else
      RETVAL = get_gamma_rice(list, k);
  OUTPUT:
    RETVAL

void
put_gamma_rice(list, k, v)
	Data::BitStream::BitList list
	int k
	unsigned long v

unsigned long
get_golomb(list, m)
	Data::BitStream::BitList list
        unsigned long m
  CODE:
    if (getpos(list) >= getlen(list))
      XSRETURN_UNDEF;
    else
      RETVAL = get_golomb(list, m);
  OUTPUT:
    RETVAL

void
put_golomb(list, m, v)
	Data::BitStream::BitList list
	unsigned long m
	unsigned long v

unsigned long
get_gamma_golomb(list, m)
	Data::BitStream::BitList list
        unsigned long m
  CODE:
    if (getpos(list) >= getlen(list))
      XSRETURN_UNDEF;
    else
      RETVAL = get_gamma_golomb(list, m);
  OUTPUT:
    RETVAL

void
put_gamma_golomb(list, m, v)
	Data::BitStream::BitList list
	unsigned long m
	unsigned long v

unsigned long
get_adaptive_gamma_rice(list, k)
	Data::BitStream::BitList list
        int &k
  CODE:
    if (getpos(list) >= getlen(list))
      XSRETURN_UNDEF;
    else
      RETVAL = get_adaptive_gamma_rice(list, &k);
  OUTPUT:
    k
    RETVAL

void
put_adaptive_gamma_rice(list, k, v)
	Data::BitStream::BitList list
	int &k
	unsigned long	 v
  OUTPUT:
    k
