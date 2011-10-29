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
  $v->put_string('111010100');
  is($v->len, 18);
  $v->setpos(0);
  my $v1 = $v->read_string(6);
  is($v1, '000101');
  my $v2 = $v->read_string(6);
  is($v2, '011111');
  my $v3 = $v->read_string(6);
  is($v3, '010100');
  $v->setpos(0);
  my $v4 = $v->read_string(18);
  is($v4, '000101011111010100');
  $v->setpos(0);
  my $v5 = $v->read(18);
  is($v5,22484);
}

done_testing;
