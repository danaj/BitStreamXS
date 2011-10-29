#!/usr/bin/perl
use strict;
use warnings;

use Test::More;
use Data::BitStream::BitList;
my $v = Data::BitStream::BitList->new;

is($v->len, 0);
is($v->pos, 0);

$v->put_evenrodeh(0 .. 257);
$v->setpos(0);
is_deeply( [$v->get_evenrodeh(-1)], [0 .. 257], 'evenrodeh 0-257');

done_testing;
