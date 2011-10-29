#!/usr/bin/perl
use strict;
use warnings;

use Test::More;
use Data::BitStream::BitList;
my $v = Data::BitStream::BitList->new(100);

is($v->getlen, 0);
is($v->getpos, 0);

my $k;

$k = 0;
foreach my $n (0 .. 257) {
  $v->put_adaptive_gamma_rice($k, $n);
}

#$v->dump();

$v->setpos(0);
$k = 0;
foreach my $n (0 .. 257) {
  my $value = $v->get_adaptive_gamma_rice($k);
  is($value, $n);
}

done_testing;
