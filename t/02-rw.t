#!/usr/bin/perl
use strict;
use warnings;

use Test::More;
use Data::BitStream::BitList;
my $v = Data::BitStream::BitList->new;

is($v->getlen, 0);
is($v->getpos, 0);

$v->vwrite(2, 3);
#$v->dump();
$v->setpos(0);
my $value = $v->vread(2);
is($value, 3);

done_testing;
