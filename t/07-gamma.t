#!/usr/bin/perl
use strict;
use warnings;

use Test::More;
use Data::BitStream::BitList;
my $v = Data::BitStream::BitList->new(100);

is($v->getlen, 0);
is($v->getpos, 0);

foreach my $n (0 .. 257) {
  $v->put_gamma($n);
}

#$v->dump();

$v->setpos(0);
foreach my $n (0 .. 257) {
  my $value = $v->get_gamma;
  is($value, $n);
}

done_testing;
