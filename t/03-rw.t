#!/usr/bin/perl
use strict;
use warnings;

use Test::More;
use Data::BitStream::BitList;
my $v = Data::BitStream::BitList->new;

is($v->len, 0);
is($v->pos, 0);

foreach my $n (1 .. 8) {
  $v->write(16, 0x4225 | ($n << 12));
}
#$v->dump();

$v->setpos(0);
foreach my $n (1 .. 8) {
  my $value = $v->read(16);
  is($value, 0x4225 | ($n << 12));
}

done_testing;
