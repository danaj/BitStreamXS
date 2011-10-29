#!/usr/bin/perl
use strict;
use warnings;

use Test::More;
use Data::BitStream::BitList;
my $v = Data::BitStream::BitList->new;

is($v->getlen, 0);
is($v->getpos, 0);

foreach my $n (0 .. 129) {
  $v->vwrite(8, $n);
}
#$v->dump();

$v->setpos(0);
foreach my $n (0 .. 129) {
  my $value = $v->vread(8);
  is($value, $n);
}

done_testing;
