
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "bitlist.h"

typedef BitList *Data__BitStream__BitList;

MODULE = Data::BitStream::BitList	PACKAGE = Data::BitStream::BitList

PROTOTYPES: ENABLE

Data::BitStream::BitList
new(package, nBits)
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

void
vwrite(list, n, v)
	Data::BitStream::BitList list
	int	 n
	unsigned long	 v

unsigned long
get_unary(list)
	Data::BitStream::BitList list

void
put_unary(list, v)
	Data::BitStream::BitList list
	unsigned long	 v

unsigned long
get_gamma(list)
	Data::BitStream::BitList list

void
put_gamma(list, v)
	Data::BitStream::BitList list
	unsigned long	 v

void
put_string(list, s)
	Data::BitStream::BitList list
	char* s
