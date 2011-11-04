#!/usr/bin/perl
use strict;
use warnings;

use Test::More;
use Data::BitStream::XS;

plan tests => 18;

eval { my $v = Data::BitStream::XS->new(mode=>'wrong'); };
like($@, qr/unknown mode/i, "Incorrect mode");

my $v = Data::BitStream::XS->new;
is($v->writing, 1, "writing mode");

eval { $v->rewind; };
like($@, qr/rewind while writing/, "rewind while writing");

eval { $v->exhausted; };
like($@, qr/exhausted while writing/, "exhausted while writing");

eval { $v->skip(3); };
like($@, qr/skip while writing/, "skip while writing");

eval { $v->read(3); };
like($@, qr/read while writing/, "read while writing");
eval { $v->readahead(3); };
like($@, qr/read while writing/, "readahead while writing");
eval { $v->read_string(3); };
like($@, qr/read while writing/, "read_string while writing");

$v->rewind_for_read;
eval { $v->read(0); };
like($@, qr/invalid bits/, "read invalid bits: 0");
eval { $v->read(-4); };
like($@, qr/invalid bits/, "read invalid bits: -4");
eval { $v->read(1025); };
like($@, qr/invalid bits/, "read invalid bits: 1025");

eval { $v->readahead(0); };
like($@, qr/invalid bits/, "readahead invalid bits: 0");
eval { $v->readahead(-4); };
like($@, qr/invalid bits/, "readahead invalid bits: -4");
eval { $v->readahead(1025); };
like($@, qr/invalid bits/, "readahead invalid bits: 1025");

eval { $v->read_string(-3); };
like($@, qr/invalid bits/, "read_string invalid bits: -3");

eval { $v->get_binword(0); };
like($@, qr/invalid parameters/, "get_binword invalid bits: 0");
eval { $v->get_binword(-4); };
like($@, qr/invalid parameters/, "get_binword invalid bits: -4");
eval { $v->get_binword(1025); };
like($@, qr/invalid parameters/, "get_binword invalid bits: 1025");

# skip off stream
# write
# put_string
