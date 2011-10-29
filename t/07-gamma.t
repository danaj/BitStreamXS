#!/usr/bin/perl
use strict;
use warnings;

use Test::More  tests => 258;

use Data::BitStream::BitList;
my $v = Data::BitStream::BitList->new;

foreach my $n (0 .. 257) {
  $v->put_gamma($n);
}

$v->rewind_for_read;
foreach my $n (0 .. 257) {
  my $value = $v->get_gamma;
  is($value, $n);
}
