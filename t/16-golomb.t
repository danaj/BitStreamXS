#!/usr/bin/perl
use strict;
use warnings;

use Test::More;
use Data::BitStream::BitList;
my $v = Data::BitStream::BitList->new;

is($v->len, 0);
is($v->pos, 0);

my @a = 0 .. 257;
my $nitems = scalar @a;
foreach my $k (0 .. 31) {
  my $m = 2*$k + 1;
  $v->put_golomb($m, @a);
}

$v->setpos(0);
foreach my $k (0 .. 31) {
  my $m = 2*$k + 1;
  is_deeply( [$v->get_golomb($m, $nitems)], \@a, "golomb($m) 0-257");
}

done_testing;
