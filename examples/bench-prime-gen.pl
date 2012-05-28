#!/usr/bin/perl -w

use strict;
use Math::Primality;
use Math::Prime::XS;
use Data::BitStream::XS;
use Math::Prime::FastSieve;
use Benchmark qw/:all/;
my $test = shift || 'range';
my $count = shift || -5;   # I use -30 for my benchmarking

die "test must be lbsr or lb or range" unless $test =~ /^(lbsr|lb|range)$/;

                       # lbsr tests 10^9 to 10^9+1000
my $lb = 1_000_000_000;
my $sr = 1000;

my @kl;
if ($test eq 'lb') {
  @kl = (10_000);      # Test 2 to 10M
} else {
  @kl = (1,10,100);    # Test 2 to 1000, 1001 to 10000, 10001 to 100000
}

# Here's a simple test:
#  >1 hr  time perl -E 'use Math::Primality qw/prime_count/; say prime_count(800_000_000);'
#  16.2s  time perl -E 'use Math::Prime::XS qw/primes/; my @primes = primes(2,800_000_000); say scalar @primes;'
#   3.9s  time perl -E 'use Math::Prime::FastSieve; my $sieve = Math::Prime::FastSieve::Sieve->new(800_000_000); say $sieve->count_sieve;'
#   1.7s  time perl -E 'use Data::BitStream::XS qw/prime_count/; say prime_count(800_000_000);'
#
# 255+s   time perl -E 'use Math::Prime::XS qw/primes/; my @primes = primes(2,10_00_000_000); say scalar @primes;'
#  64.5s  time perl -E 'use Math::Prime::FastSieve; my $sieve = Math::Prime::FastSieve::Sieve->new(10_000_000_000); say $sieve->count_sieve;'
#  24.3s  time perl -E 'use Data::BitStream::XS qw/prime_count/; say prime_count(10_000_000_000);'


sub gen_primexs {
  my($p, $start, $end) = @_;
  push @{$p}, Math::Prime::XS::sieve_primes($start, $end);
  #push @{$p}, Math::Prime::XS::trial_primes($start, $endf);
  1;
}
sub gen_fastsieve {
  my($p, $start, $end) = @_;
  my $sieve = Math::Prime::FastSieve::Sieve->new($end);
  push @{$p}, @{$sieve->ranged_primes($start, $end)};
  1;
}
sub gen_primality {
  my($p, $start, $end) = @_;
  my $nextprime = Math::Primality::next_prime($start);
  while ($nextprime <= $end) {
    push @{$p}, $nextprime;
    $nextprime = Math::Primality::next_prime($nextprime);
  }
  1;
}
sub gen_dbxs_next {
  my($p, $start, $end) = @_;
  my $nextprime = Data::BitStream::XS::next_prime($start);
  while ($nextprime <= $end) {
    push @{$p}, $nextprime;
    $nextprime = Data::BitStream::XS::next_prime($nextprime);
  }
  1;
}
sub gen_dbxs_erat {
  my($p, $start, $end) = @_;
  push @{$p}, @{Data::BitStream::XS::primes({method=>'erat'},$start, $end)};
  1;
}
sub gen_dbxs_simple {
  my($p, $start, $end) = @_;
  push @{$p}, @{Data::BitStream::XS::primes({method=>'simple'},$start, $end)};
  1;
}
sub gen_pureperl {
  my($p, $start, $end) = @_;
  my $nextprime = _next_prime($start);
  while ($nextprime <= $end) {
    push @{$p}, $nextprime;
    $nextprime = _next_prime($nextprime);
  }
  1;
}

if ($test eq 'lbsr') {
  cmpthese($count,{
    'MPXS'         => sub { my @p; gen_primexs(\@p, $lb, $lb+$sr); },
    'MPFS'         => sub { my @p; gen_fastsieve(\@p, $lb, $lb+$sr); },
    'DBXS erat'    => sub { my @p; gen_dbxs_erat(\@p, $lb, $lb+$sr); },
    'DBXS simple'  => sub { my @p; gen_dbxs_simple(\@p, $lb, $lb+$sr); },
    'DBXS next'    => sub { my @p; gen_dbxs_next(\@p, $lb, $lb+$sr); },
    'Pure Perl'    => sub { my @p; gen_pureperl(\@p, $lb, $lb+$sr); },
    'Math::Primality' => sub { my @p; gen_primality(\@p, $lb, $lb+$sr); },
  });
} else {
  cmpthese($count,{
    'MPXS'         => sub { my @p = (2);
                            gen_primexs(\@p,$p[-1]+1,$_*1000+3) for (@kl);
                          },
    'MPFS'         => sub { my @p = (2);
                            gen_fastsieve(\@p,$p[-1]+1,$_*1000+3) for (@kl);
                          },
    'DBXS erat'    => sub { my @p = (2);
                            gen_dbxs_erat(\@p,$p[-1]+1,$_*1000+3) for (@kl);
                          },
    'DBXS simple'  => sub { my @p = (2);
                            gen_dbxs_simple(\@p,$p[-1]+1,$_*1000+3) for (@kl);
                          },
    'DBXS next'    => sub { my @p = (2);
                            gen_dbxs_next(\@p,$p[-1]+1,$_*1000+3) for (@kl);
                          },
    'Pure Perl'    => sub { my @p = (2);
                            gen_pureperl(\@p,$p[-1]+1,$_*1000+3) for (@kl);
                          },
    'Math::Primality' => sub { my @p = (2);
                            gen_primality(\@p,$p[-1]+1,$_*1000+3) for (@kl);
                          },
  });

}


##########################   Pure Perl generation

  sub _is_prime {   # Note:  assumes n is not divisible by 2, 3, or 5!
    my $x = shift;
    my $q;
    # Quick loop for small prime divisibility
    foreach my $i (7, 11, 13, 17, 19, 23, 29) {
      $q = int($x/$i); return 1 if $q < $i; return 0 if $x == ($q*$i);
    }
    # Unrolled mod-30 loop
    my $i = 31;
    while (1) {
      $q = int($x/$i); return 1 if $q < $i; return 0 if $x == ($q*$i);  $i += 6;
      $q = int($x/$i); return 1 if $q < $i; return 0 if $x == ($q*$i);  $i += 4;
      $q = int($x/$i); return 1 if $q < $i; return 0 if $x == ($q*$i);  $i += 2;
      $q = int($x/$i); return 1 if $q < $i; return 0 if $x == ($q*$i);  $i += 4;
      $q = int($x/$i); return 1 if $q < $i; return 0 if $x == ($q*$i);  $i += 2;
      $q = int($x/$i); return 1 if $q < $i; return 0 if $x == ($q*$i);  $i += 4;
      $q = int($x/$i); return 1 if $q < $i; return 0 if $x == ($q*$i);  $i += 6;
      $q = int($x/$i); return 1 if $q < $i; return 0 if $x == ($q*$i);  $i += 2;
    }
    1;
  }

  sub _next_prime {
    my $x = shift;
    if ($x <= 67) {
      return (2,2,3,5,5,7,7,11,11,11,11,13,13,17,17,17,17,19,19,
              23,23,23,23,29,29,29,29,29,29,31,31,37,37,37,37,37,
              37,41,41,41,41,43,43,47,47,47,47,53,53,53,53,53,53,
              59,59,59,59,59,59,61,61,67,67,67,67,67,67,71)[$x];
    }
    my @_prime_indices = (1, 7, 11, 13, 17, 19, 23, 29);
    $x += 1;
    my $k0 = int($x/30);
    my $in = 0;  $in++ while ($x-$k0*30) > $_prime_indices[$in];
    my $n = 30 * $k0 + $_prime_indices[$in];
    while (!_is_prime($n)) {
      if (++$in == 8) {  $k0++; $in = 0;  }
      $n = 30 * $k0 + $_prime_indices[$in];
    }
    $n;
  }


