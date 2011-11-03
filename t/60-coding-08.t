#!/usr/bin/perl
use strict;
use warnings;

use Test::More;
use Data::BitStream::XS qw(code_is_universal);
my @encodings = qw|
              Unary Gamma Delta Omega
              Fibonacci EvenRodeh Levenstein
              Golomb(10) Golomb(16) Golomb(14000)
              Rice(2) Rice(9)
              GammaGolomb(3) GammaGolomb(128) ExpGolomb(5)
              BoldiVigna(2) Baer(0) Baer(-2) Baer(2)
              StartStepStop(3-3-99) StartStop(1-0-1-0-2-12-99)
            |;

plan tests => scalar @encodings;

my @fibs = (0,1,1);
{
  my ($v2, $v1) = ( $fibs[-2], $fibs[-1] );
  while ($fibs[-1] < ~0) {
    ($v2, $v1) = ($v1, $v2+$v1);
    push(@fibs, $v1);
  }
}

my $nvals = 500;
my @data;
for (1 .. $nvals) {
  push @data, int(rand(100_000));
}
foreach my $encoding (@encodings) {
  my $stream = Data::BitStream::XS->new;

  my $nfibs = 30;
  if (code_is_universal($encoding)) {
    $nfibs = ($stream->maxbits < 64)  ?  47  :  80;
  }
  my @data = @fibs;
  $#data = $nfibs;

  $stream->code_put($encoding, @data);
  $stream->rewind_for_read;
  my @v = $stream->code_get($encoding, -1);
  is_deeply( \@v, \@data, "encoded F[0]-F[$nfibs] using $encoding");
}