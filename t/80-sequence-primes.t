#!/usr/bin/perl
use strict;
use warnings;

use Test::More;
use Data::BitStream::XS qw(is_prime next_prime primes primes prime_count prime_count_lower prime_count_upper prime_count_approx);

my $use64 = Data::BitStream::XS->maxbits > 32;
my $extra = defined $ENV{RELEASE_TESTING} && $ENV{RELEASE_TESTING};

plan tests => 6 + 19 + 3573 + (5 + 29 + 22 + 23 + 16) + 499 + 4*12+1+1
              + 10*3+7
              + ($use64 ? 5 + 9*2 : 0)
              + ($extra ? 3 : 0);

# These simple prime tests mostly taken from from Math::Primality.

ok( is_prime(2), '2 is prime');
ok(!is_prime(1), '1 is not prime');
ok(!is_prime(0), '0 is not prime');
ok(!is_prime(-1), '-1 is not prime');
ok(!is_prime(-2), '-2 is not prime');
ok( !is_prime(20), "20 is not prime");

# powers of 2
foreach my $k (2 .. 20) {
  my $k2 = 2**$k;
  ok(!is_prime($k2), "2**$k=$k2 is not prime");
}

my @small_primes = qw/
2 3 5 7 11 13 17 19 23 29 31 37 41 43 47 53 59 61 67 71
73 79 83 89 97 101 103 107 109 113 127 131 137 139 149 151 157 163 167 173
179 181 191 193 197 199 211 223 227 229 233 239 241 251 257 263 269 271 277 281
283 293 307 311 313 317 331 337 347 349 353 359 367 373 379 383 389 397 401 409
419 421 431 433 439 443 449 457 461 463 467 479 487 491 499 503 509 521 523 541
547 557 563 569 571 577 587 593 599 601 607 613 617 619 631 641 643 647 653 659
661 673 677 683 691 701 709 719 727 733 739 743 751 757 761 769 773 787 797 809
811 821 823 827 829 839 853 857 859 863 877 881 883 887 907 911 919 929 937 941
947 953 967 971 977 983 991 997 1009 1013 1019 1021 1031 1033 1039 1049 1051 1061 1063 1069
1087 1091 1093 1097 1103 1109 1117 1123 1129 1151 1153 1163 1171 1181 1187 1193 1201 1213 1217 1223
1229 1231 1237 1249 1259 1277 1279 1283 1289 1291 1297 1301 1303 1307 1319 1321 1327 1361 1367 1373
1381 1399 1409 1423 1427 1429 1433 1439 1447 1451 1453 1459 1471 1481 1483 1487 1489 1493 1499 1511
1523 1531 1543 1549 1553 1559 1567 1571 1579 1583 1597 1601 1607 1609 1613 1619 1621 1627 1637 1657
1663 1667 1669 1693 1697 1699 1709 1721 1723 1733 1741 1747 1753 1759 1777 1783 1787 1789 1801 1811
1823 1831 1847 1861 1867 1871 1873 1877 1879 1889 1901 1907 1913 1931 1933 1949 1951 1973 1979 1987
1993 1997 1999 2003 2011 2017 2027 2029 2039 2053 2063 2069 2081 2083 2087 2089 2099 2111 2113 2129
2131 2137 2141 2143 2153 2161 2179 2203 2207 2213 2221 2237 2239 2243 2251 2267 2269 2273 2281 2287
2293 2297 2309 2311 2333 2339 2341 2347 2351 2357 2371 2377 2381 2383 2389 2393 2399 2411 2417 2423
2437 2441 2447 2459 2467 2473 2477 2503 2521 2531 2539 2543 2549 2551 2557 2579 2591 2593 2609 2617
2621 2633 2647 2657 2659 2663 2671 2677 2683 2687 2689 2693 2699 2707 2711 2713 2719 2729 2731 2741
2749 2753 2767 2777 2789 2791 2797 2801 2803 2819 2833 2837 2843 2851 2857 2861 2879 2887 2897 2903
2909 2917 2927 2939 2953 2957 2963 2969 2971 2999 3001 3011 3019 3023 3037 3041 3049 3061 3067 3079
3083 3089 3109 3119 3121 3137 3163 3167 3169 3181 3187 3191 3203 3209 3217 3221 3229 3251 3253 3257
3259 3271 3299 3301 3307 3313 3319 3323 3329 3331 3343 3347 3359 3361 3371 3373 3389 3391 3407 3413
3433 3449 3457 3461 3463 3467 3469 3491 3499 3511 3517 3527 3529 3533 3539 3541 3547 3557 3559 3571
/;
my %small_primes;
map { $small_primes{$_} = 1; } @small_primes;

foreach my $n (0 .. 3572) {
  if (defined $small_primes{$n}) {
    ok(is_prime($n), "$n is prime");
  } else {
    ok(!is_prime($n), "$n is not prime");
  }
}

map { ok(!is_prime($_), "A006945 number $_ is not prime") }
  qw/9 2047 1373653 25326001 3215031751/;
map { ok(!is_prime($_), "A006945 number $_ is not prime") }
  qw/2152302898747 3474749660383 341550071728321 341550071728321 3825123056546413051/ if $use64;

map { ok(!is_prime($_), "Carmichael Number $_ is not prime") }
  qw/561 1105 1729 2465 2821 6601 8911 10585 15841 29341 41041 46657 52633
     62745 63973 75361 101101 340561 488881 852841 1857241 6733693
     9439201 17236801 23382529 34657141 56052361 146843929 216821881/;

map { ok(!is_prime($_), "Pseudoprime (base 2) $_ is not prime" ) }
  qw/341 561 645 1105 1387 1729 1905 2047 2465 2701 2821 3277 4033 4369 4371
     4681 5461 6601 7957 8321 52633 88357/;

map { ok(!is_prime($_), "Pseudoprime (base 3) $_ is not prime" ) }
  qw/121 703 1891 3281 8401 8911 10585 12403 16531 18721 19345 23521 31621
     44287 47197 55969 63139 74593 79003 82513 87913 88573 97567/;

map { ok(!is_prime($_), "Pseudoprime (base 5) $_ is not prime" ) }
  qw/781 1541 5461 5611 7813 13021 14981 15751 24211 25351 29539 38081
     40501 44801 53971 79381/;

# End of tests derived from Math::Primality test ideas

# Next prime
for (my $i = 0; $i < (scalar @small_primes) - 1; $i++) {
  my $n = next_prime($small_primes[$i]);
  is("$n", "$small_primes[$i+1]", "the next prime after $small_primes[$i] is $small_primes[$i+1] ?= $n");
}

# Ranges
foreach my $method (qw/trial erat simple sieve/) {
  is_deeply( primes({method=>$method}, 0, 3572), \@small_primes, "Primes between 0 and 3572" );
  is_deeply( primes({method=>$method}, 2, 20), [2,3,5,7,11,13,17,19], "Primes between 2 and 20" );
  is_deeply( primes({method=>$method}, 30, 70), [31,37,41,43,47,53,59,61,67], "Primes between 30 and 70" );
  is_deeply( primes({method=>$method}, 30, 70), [31,37,41,43,47,53,59,61,67], "Primes between 30 and 70" );
  is_deeply( primes({method=>$method}, 20, 2), [], "Primes between 20 and 2" );
  is_deeply( primes({method=>$method}, 2, 2), [2], "Primes ($method) between 2 and 2" );
  is_deeply( primes({method=>$method}, 3, 3), [3], "Primes between 3 and 3" );
  is_deeply( primes({method=>$method}, 2010733, 2010733+148), [2010733,2010733+148], "Primegap 21 inclusive" );
  is_deeply( primes({method=>$method}, 2010733+1, 2010733+148-2), [], "Primegap 21 exclusive" );
  is_deeply( primes({method=>$method}, 3088, 3164), [3089,3109,3119,3121,3137,3163], "Primes between 3088 and 3164" );
  is_deeply( primes({method=>$method}, 3089, 3163), [3089,3109,3119,3121,3137,3163], "Primes between 3089 and 3163" );
  is_deeply( primes({method=>$method}, 3090, 3162), [3109,3119,3121,3137], "Primes between 3090 and 3162" );
}

# Compare the two sieves
is_deeply( primes({method=>'erat'}, 0, 1000000), primes({method=>'simpleerat'}, 0, 1000000), "Compare sieves" );

# Large 32-bit gap using trial
is_deeply( primes({method=>'trial'}, 3842610773, 3842610773+336), [3842610773,3842610773+336], "Primegap 34 inclusive" );


# Prime counts
my %pivals32 = (
                  1 => 0,
                 10 => 4,
                100 => 25,
               1000 => 168,
              10000 => 1229,
             100000 => 9592,
            1000000 => 78498,
           10000000 => 664579,
          100000000 => 5761455,
         1000000000 => 50847534,
);
my %pivals64 = (
        10000000000 => 455052511,
       100000000000 => 4118054813,
      1000000000000 => 37607912018,
     10000000000000 => 346065536839,
    100000000000000 => 3204941750802,
   1000000000000000 => 29844570422669,
  10000000000000000 => 279238341033925,
 100000000000000000 => 2623557157654233,
1000000000000000000 => 24739954287740860,
);
while (my($n, $pin) = each (%pivals32)) {
  cmp_ok( prime_count_upper($n), '>=', $pin, "Pi($n) <= upper estimate" );
  cmp_ok( prime_count_lower($n), '<=', $pin, "Pi($n) >= lower estimate" );
  if ( ($n <= 2000000) || $extra ) {
    is( prime_count($n), $pin, "Pi($n) = $pin" );
  }
  my $approx_range = abs($pin - prime_count_approx($n));
  my $range_limit = 1100;
  cmp_ok( $approx_range, '<=', $range_limit, "prime_count_approx($n) within $range_limit");
}
if ($use64) {
  while (my($n, $pin) = each (%pivals64)) {
    cmp_ok( prime_count_upper($n), '>=', $pin, "Pi($n) <= upper estimate" );
    cmp_ok( prime_count_lower($n), '<=', $pin, "Pi($n) >= lower estimate" );
  }
}
