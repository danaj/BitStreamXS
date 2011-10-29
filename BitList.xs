
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
reserve_bits(list, n)
	Data::BitStream::BitList list
	int	 n

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
vread(list, n)
	Data::BitStream::BitList list
	int	 n
  CODE:
    if (getpos(list) >= getlen(list))
      XSRETURN_UNDEF;
    else
      RETVAL = vread(list,n);
  OUTPUT:
    RETVAL

unsigned long
vreadahead(list, n)
	Data::BitStream::BitList list
	int	 n
  CODE:
    if (getpos(list) >= getlen(list))
      XSRETURN_UNDEF;
    else
      RETVAL = vreadahead(list,n);
  OUTPUT:
    RETVAL

void
vwrite(list, n, v)
	Data::BitStream::BitList list
	int	 n
	unsigned long	 v

void
put_string(list, s)
	Data::BitStream::BitList list
	char* s



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

