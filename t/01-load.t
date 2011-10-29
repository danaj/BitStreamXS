#!/usr/bin/perl
use strict;
use warnings;

use Test::More;

require_ok 'Data::BitStream::BitList';

can_ok('Data::BitStream::BitList' => 'new');
#my $a = new Set::Bit 100;

my $v = new_ok('Data::BitStream::BitList' => [100]);

is($v->len, 0);
is($v->pos, 0);

$v->write(10, 5);

done_testing;
