package Data::BitStream::BitList;
use strict;
use warnings;

use parent qw( Exporter );
our @EXPORT_OK = qw();

our $VERSION;
BEGIN {
  $VERSION = '0.01';
  eval {
    require XSLoader;
    XSLoader::load(__PACKAGE__, $VERSION);
    1;
  } or do {
    warn "Using PP BitList";
    die "Not implemented";
  }
}

#require DynaLoader;
#our @ISA;
#push @ISA, 'DynaLoader';
#__PACKAGE__->bootstrap($VERSION);

# Could put subs here

1;
