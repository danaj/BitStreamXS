#!/usr/bin/perl
use strict;
use warnings;

use Test::More  tests => 1;

use Data::BitStream::BitList;
my $v = Data::BitStream::BitList->new;

$v->put_omega(0 .. 257);
$v->rewind_for_read;
is_deeply( [$v->get_omega(-1)], [0 .. 257], 'omega 0-257');
