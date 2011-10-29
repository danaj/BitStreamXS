#!/usr/bin/perl
use strict;
use warnings;

use Test::More;
use Data::BitStream::BitList;
my $v = Data::BitStream::BitList->new(100);

is($v->getlen, 0);
is($v->getpos, 0);

foreach my $n (1 .. 8) {
  $v->vwrite(16, 0x4225 | ($n << 12));
}
#$v->dump();

$v->setpos(0);
foreach my $n (1 .. 8) {
  my $value = $v->vread(16);
  is($value, 0x4225 | ($n << 12));
}

done_testing;
