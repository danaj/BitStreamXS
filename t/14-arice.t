#!/usr/bin/perl
use strict;
use warnings;

use Test::More;
use Data::BitStream::BitList;
my $v = Data::BitStream::BitList->new;

is($v->len, 0);
is($v->pos, 0);

my $k;

my @a = 0 .. 257;
my $nitems = scalar @a;

$k = 0;
$v->put_adaptive_gamma_rice($k, @a);
my $endk = $k;
isnt($endk, 0, "endk ($endk) isn't 0");

$v->setpos(0);
$k = 0;
#my @values = $v->get_adaptive_gamma_rice($k, $nitems);
  my @values;
  push @values, $v->get_adaptive_gamma_rice($k,2)  for (1 .. $nitems/2);
is($k, $endk, "endk ($endk) matches");
is_deeply( [@values], \@a, "arice get array 0-257");

# Now test one at a time.
{
  $k = 0;
  foreach my $n (0 .. 257) {
    $v->put_adaptive_gamma_rice($k, $n);
  }
  is($k, $endk, "endk ($endk) matches");

  $v->setpos(0);
  $k = 0;
  my @values;
  foreach my $n (0 .. 257) {
    push @values, $v->get_adaptive_gamma_rice($k);
  }
  is_deeply( \@values, \@a, "arice single get/put 0-257");
  is($k, $endk, "endk ($endk) matches");
}

done_testing;
