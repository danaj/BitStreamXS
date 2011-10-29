#!/usr/bin/perl
use strict;
use warnings;

use Test::More;
use Data::BitStream::BitList;
my $v = Data::BitStream::BitList->new;

is($v->len, 0);
is($v->pos, 0);

$v->write(2, 3);
#$v->dump();
$v->setpos(0);
my $value = $v->read(2);
is($value, 3);

done_testing;
