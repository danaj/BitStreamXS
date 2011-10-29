#!/usr/bin/perl
use strict;
use warnings;

use Test::More;
use Data::BitStream::BitList;
my $v = Data::BitStream::BitList->new(100);

is($v->getlen, 0);
is($v->getpos, 0);

foreach my $b (8 .. 64) {
  foreach my $n (0 .. 129) {
    $v->vwrite($b, $n);
  }
}
#$v->dump();

$v->setpos(0);
foreach my $b (8 .. 64) {
  foreach my $n (0 .. 129) {
    my $value = $v->vread($b);
    is($value, $n);
  }
}

done_testing;
