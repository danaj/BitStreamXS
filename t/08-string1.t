#!/usr/bin/perl
use strict;
use warnings;

use Test::More;
use Data::BitStream::BitList;
my $v = Data::BitStream::BitList->new(100);

is($v->getlen, 0);
is($v->getpos, 0);

{
  $v->put_string('000101011');
  is($v->getlen, 9);
  $v->setpos(0);
  my $value = $v->vread(9);
  is($value, 43);
}

done_testing;
