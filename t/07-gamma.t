#!/usr/bin/perl
use strict;
use warnings;

use Test::More;
use Data::BitStream::BitList;
my $v = Data::BitStream::BitList->new;

is($v->len, 0);
is($v->pos, 0);

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
