#!/usr/bin/perl
use strict;
use warnings;

use Test::More  tests => 5;

require_ok 'Data::BitStream::BitList';

can_ok('Data::BitStream::BitList' => 'new');

#my $v = new_ok('Data::BitStream::BitList');
my $v = Data::BitStream::BitList->new;
isa_ok $v, 'Data::BitStream::BitList';

is($v->len, 0, 'starting len 0');
is($v->pos, 0, 'starting pos 0');

$v->write(10, 5);

#done_testing;
