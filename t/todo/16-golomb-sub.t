#!/usr/bin/perl
use strict;
use warnings;

use Test::More  tests => 32;

use Data::BitStream::XS;
my $v = Data::BitStream::XS->new;

my @a = 0 .. 257;
my $nitems = scalar @a;
foreach my $k (0 .. 31) {
  my $m = 2*$k + 1;
  #$v->put_golomb_sub(sub { shift->put_delta(@_); }, $m, @a);
  $v->put_golomb_sub(
    sub { my $self = shift;
          isa_ok $self, 'Data::BitStream::XS';
          #die unless $v == $self;
          die unless $self->writing;
          $self->put_delta(@_); },
    $m, @a);
}

$v->rewind_for_read;
foreach my $k (0 .. 31) {
  my $m = 2*$k + 1;
  my @v = $v->get_golomb_sub(sub { shift->get_delta(@_); }, $m, $nitems);
  is_deeply( \@v, \@a, "golomb($m) 0-257");
}
