#!/usr/bin/perl
use strict;
use warnings;

eval {require List::Util; 1;} or do {
  sub shuffle (@) {
    my @a=\(@_);
    my $n;
    my $i=@_;
    map {
      $n = rand($i--);
      (${$a[$n]}, $a[$n] = $a[$i])[0];
    } @_;
  }
};
use Test::More;
use Data::BitStream::XS qw(code_is_universal);
my @encodings = qw|
              Unary Unary1 Gamma Delta Omega
              Fibonacci EvenRodeh Levenstein
              Golomb(10) Golomb(16) Golomb(14000)
              Rice(2) Rice(9)
              GammaGolomb(3) GammaGolomb(128) ExpGolomb(5)
              BoldiVigna(2) Baer(0) Baer(-2) Baer(2)
              StartStepStop(3-3-99) StartStop(1-0-1-0-2-12-99)
              ARice(2)
            |;

# Perl 5.6.2 64-bit support is problematic
my $maxval = ($] < 5.8)  ?  0xFFFFFFFF  :  ~0;
my @maxdata = (0, 1, 2, 33, 65, 129,
               ($maxval >> 1) - 2,
               ($maxval >> 1) - 1,
               ($maxval >> 1),
               ($maxval >> 1) + 1,
               ($maxval >> 1) + 2,
               $maxval-2,
               $maxval-1,
               $maxval,
              );
push @maxdata, @maxdata;
my @data = shuffle @maxdata;

# Remove encodings that can't encode ~0
@encodings = grep { code_is_universal($_) } @encodings;
#@encodings = grep { $_ !~ /^(Omega)/i } @encodings;

plan tests => scalar @encodings;

foreach my $encoding (@encodings) {
  my $stream = Data::BitStream::XS->new;
  $stream->code_put($encoding, @data);
  $stream->rewind_for_read;
  my @v = $stream->code_get($encoding, -1);
  is_deeply( \@v, \@data, "range test using $encoding");
}
