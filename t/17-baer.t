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
foreach my $k (-32 .. 32) {
  $v->put_baer($k, @a);
}

$v->setpos(0);
foreach my $k (-32 .. 32) {
  is_deeply( [$v->get_baer($k, $nitems)], \@a, "baer($k) 0-257");
}

done_testing;
