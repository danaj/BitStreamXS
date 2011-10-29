#!/usr/bin/perl
use strict;
use warnings;

use Test::More;
use Data::BitStream::BitList;
my $v = Data::BitStream::BitList->new;

is($v->len, 0);
is($v->pos, 0);

my @a = 0 .. 255;
my $nitems = scalar @a;
foreach my $k (8 .. $v->maxbits) {
  $v->put_binword($k, @a);
}

$v->setpos(0);
foreach my $k (8 .. $v->maxbits) {
  is_deeply( [$v->get_binword($k, $nitems)], \@a, "binword($k) 0-255");
}

done_testing;
