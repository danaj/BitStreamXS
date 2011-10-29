#!/usr/bin/perl
use strict;
use warnings;

use Test::More;
use Data::BitStream::BitList;
my $v = Data::BitStream::BitList->new;

is($v->getlen, 0);
is($v->getpos, 0);

foreach my $k (0 .. 31) {
  foreach my $n (0 .. 257) {
    my $m = 2*$k+1;
    $v->put_golomb($m, $n);
  }
}

#$v->dump();

$v->setpos(0);
foreach my $k (0 .. 31) {
  foreach my $n (0 .. 257) {
    my $m = 2*$k+1;
    my $value = $v->get_golomb($m);
    is($value, $n);
  }
}

done_testing;
