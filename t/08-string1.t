#!/usr/bin/perl
use strict;
use warnings;

use Test::More  tests => 2;

use Data::BitStream::BitList;
my $v = Data::BitStream::BitList->new;

{
  $v->put_string('000101011');
  is($v->len, 9);
  $v->rewind_for_read;
  my $value = $v->read(9);
  is($value, 43);
}
