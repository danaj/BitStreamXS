#!/usr/bin/perl
use strict;
use warnings;

use Test::More;

require_ok 'Data::BitStream::BitList';

can_ok('Data::BitStream::BitList' => 'new');
#my $a = new Set::Bit 100;

my $v = new_ok('Data::BitStream::BitList' => [100]);

is($v->getlen, 0);
is($v->getpos, 0);

$v->vwrite(10, 5);

done_testing;
