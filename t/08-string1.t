#!/usr/bin/perl
use strict;
use warnings;

use Test::More;
use Data::BitStream::BitList;
my $v = Data::BitStream::BitList->new;

is($v->len, 0);
is($v->pos, 0);

{
  $v->put_string('000101011');
  is($v->len, 9);
  $v->setpos(0);
  my $value = $v->read(9);
  is($value, 43);
}

done_testing;
