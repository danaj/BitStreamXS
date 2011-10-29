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
    $v->put_rice($k, $n);
  }
}

#$v->dump();

$v->setpos(0);
foreach my $k (0 .. 31) {
  foreach my $n (0 .. 257) {
    my $value = $v->get_rice($k);
    is($value, $n);
  }
}

done_testing;
