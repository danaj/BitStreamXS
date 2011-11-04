package Data::BitStream::XS;
# Tested with Perl 5.6.2 through 5.15.2.
# Tested on 32-bit big-endian and 64-bit little endian
use strict;
use warnings;
BEGIN {
  $Data::BitStream::XS::AUTHORITY = 'cpan:DANAJ';
}
BEGIN {
  $Data::BitStream::XS::VERSION = '0.01';
}

# parent is cleaner, and in the Perl 5.10.1 / 5.12.0 core, but not earlier.
# use parent qw( Exporter );
use base qw( Exporter );
our @EXPORT_OK = qw( code_is_supported code_is_universal );

BEGIN {
  eval {
    require XSLoader;
    XSLoader::load(__PACKAGE__, $Data::BitStream::XS::VERSION);
    1;
  } or do {
    # We could insert a Pure Perl implementation here.
    die "XS Code not available";
  }
}

################################################################################
#
#                               SUPPORT FUNCTIONS
#
################################################################################

sub erase_for_write {
  my $self = shift;
  $self->erase;
  $self->write_open if !$self->writing;
}
sub rewind_for_read {
  my $self = shift;
  $self->write_close if $self->writing;
  $self->rewind;
}

sub to_string {
  my $self = shift;
  $self->rewind_for_read;
  $self->read_string($self->len);
}

sub from_string {
  my $self = shift;
  $self->erase_for_write;
  $self->put_string($_[0]);
  $self->rewind_for_read;
}

# TODO:
sub to_store {
  shift->to_raw(@_);
}
sub from_store {
  shift->from_raw(@_);
}

# Takes a stream and inserts its contents into the current stream.
# Non-destructive to both streams.
sub put_stream {
  my $self = shift;
  my $source = shift;
  return 0 unless defined $source;

  if (ref $source eq __PACKAGE__) {
    # optimized method for us.
    $self->xput_stream($source);
  } else {
    return 0 unless $source->can('to_string');
    $self->put_string($source->to_string);
  }
  1;
}

################################################################################
#
#                                   CODES
#
################################################################################

sub get_golomb {
  my $self = shift;
  return    (ref $_[0] eq 'CODE')
         ?  $self->get_golomb_sub(@_)
         :  $self->get_golomb_sub(undef, @_);
}
sub put_golomb {
  my $self = shift;
  return    (ref $_[0] eq 'CODE')
         ?  $self->put_golomb_sub(@_)
         :  $self->put_golomb_sub(undef, @_);
}

sub get_rice {
  my $self = shift;
  return    (ref $_[0] eq 'CODE')
         ?  $self->get_rice_sub(@_)
         :  $self->get_rice_sub(undef, @_);
}
sub put_rice {
  my $self = shift;
  return    (ref $_[0] eq 'CODE')
         ?  $self->put_rice_sub(@_)
         :  $self->put_rice_sub(undef, @_);
}

sub get_arice {
  my $self = shift;
  return    (ref $_[0] eq 'CODE')
         ?  $self->get_arice_sub(@_)
         :  $self->get_arice_sub(undef, @_);
}
sub put_arice {
  my $self = shift;
  return    (ref $_[0] eq 'CODE')
         ?  $self->put_arice_sub(@_)
         :  $self->put_arice_sub(undef, @_);
}

sub get_adaptive_gamma_rice { shift->get_arice(@_); }
sub put_adaptive_gamma_rice { shift->put_arice(@_); }


# Map Start-Step-Stop codes to Start/Stop codes.
# See Data::BitStream::Code::StartStop for more detail

sub _map_sss_to_ss {
  my($start, $step, $stop, $maxstop) = @_;
  $stop = $maxstop if (!defined $stop) || ($stop > $maxstop);
  die "invalid parameters" unless ($start >= 0) && ($start <= $maxstop);
  die "invalid parameters" unless $step >= 0;
  die "invalid parameters" unless $stop >= $start;
  return if $start == $stop;  # Binword
  return if $step == 0;       # Rice

  my @pmap = ($start);
  my $blen = $start;
  while ($blen < $stop) {
    $blen += $step;
    $blen = $stop if $blen > $stop;
    push @pmap, $step;
  }
  @pmap;
}

sub put_startstepstop {
  my $self = shift;
  my $p = shift;
  die "invalid parameters" unless (ref $p eq 'ARRAY') && scalar @$p == 3;

  my($start, $step, $stop) = @$p;
  return $self->put_binword($start, @_) if $start == $stop;
  return $self->put_rice($start, @_)    if $step == 0;
  my @pmap = _map_sss_to_ss($start, $step, $stop, $self->maxbits);
  die "unexpected death" unless scalar @pmap >= 2;
  $self->put_startstop( [@pmap], @_ );
}
sub get_startstepstop {
  my $self = shift;
  my $p = shift;
  die "invalid parameters" unless (ref $p eq 'ARRAY') && scalar @$p == 3;

  my($start, $step, $stop) = @$p;
  return $self->get_binword($start, @_) if $start == $stop;
  return $self->get_rice($start, @_)    if $step == 0;
  my @pmap = _map_sss_to_ss($start, $step, $stop, $self->maxbits);
  die "unexpected death" unless scalar @pmap >= 2;
  return $self->get_startstop( [@pmap], @_ );
}

################################################################################
#
#                               TEXT METHODS
#
################################################################################

# The Data::BitStream class does this all dynamically and gets its info from
# all Data::BitStream::Code::* files that have been loaded as roles.
# We're going to do it all statically, which isn't nearly as cool.

my @_initinfo = (
    { package   => __PACKAGE__,
      name      => 'Unary',
      universal => 0,
      params    => 0,
      encodesub => sub {shift->put_unary(@_)},
      decodesub => sub {shift->get_unary(@_)}, },
    { package   => __PACKAGE__,
      name      => 'Unary1',
      universal => 0,
      params    => 0,
      encodesub => sub {shift->put_unary1(@_)},
      decodesub => sub {shift->get_unary1(@_)}, },
    { package   => __PACKAGE__,
      name      => 'BinWord',
      universal => 0,  # it is universal if and only if param == maxbits
      params    => 1,
      encodesub => sub {shift->put_binword(@_)},
      decodesub => sub {shift->get_binword(@_)}, },
    { package   => __PACKAGE__,
      name      => 'Gamma',
      universal => 1,
      params    => 0,
      encodesub => sub {shift->put_gamma(@_)},
      decodesub => sub {shift->get_gamma(@_)}, },
    { package   => __PACKAGE__,
      name      => 'Delta',
      universal => 1,
      params    => 0,
      encodesub => sub {shift->put_delta(@_)},
      decodesub => sub {shift->get_delta(@_)}, },
    { package   => __PACKAGE__,
      name      => 'Omega',
      universal => 1,
      params    => 0,
      encodesub => sub {shift->put_omega(@_)},
      decodesub => sub {shift->get_omega(@_)}, },
    { package   => __PACKAGE__,
      name      => 'EvenRodeh',
      universal => 1,
      params    => 0,
      encodesub => sub {shift->put_evenrodeh(@_)},
      decodesub => sub {shift->get_evenrodeh(@_)}, },
    { package   => __PACKAGE__,
      name      => 'Levenstein',
      aliases   => ['Levenshtein'],
      universal => 1,
      params    => 0,
      encodesub => sub {shift->put_levenstein(@_)},
      decodesub => sub {shift->get_levenstein(@_)}, },
    { package   => __PACKAGE__,
      name      => 'Fibonacci',
      universal => 1,
      params    => 0,
      encodesub => sub {shift->put_fib(@_)},
      decodesub => sub {shift->get_fib(@_)}, },
    { package   => __PACKAGE__,
      name      => 'Golomb',
      universal => 0,
      params    => 1,
      encodesub => sub {shift->put_golomb(@_)},
      decodesub => sub {shift->get_golomb(@_)}, },
    { package   => __PACKAGE__,
      name      => 'Rice',
      universal => 0,
      params    => 1,
      encodesub => sub {shift->put_rice(@_)},
      decodesub => sub {shift->get_rice(@_)}, },
    { package   => __PACKAGE__,
      name      => 'ExpGolomb',
      universal => 1,
      params    => 1,
      encodesub => sub {shift->put_expgolomb(@_)},
      decodesub => sub {shift->get_expgolomb(@_)}, },
    { package   => __PACKAGE__,
      name      => 'GammaGolomb',
      universal => 1,
      params    => 1,
      encodesub => sub {shift->put_gammagolomb(@_)},
      decodesub => sub {shift->get_gammagolomb(@_)}, },
    { package   => __PACKAGE__,
      name      => 'ARice',
      universal => 1,
      params    => 1,
      encodesub => sub {shift->put_arice(@_)},
      decodesub => sub {shift->get_arice(@_)}, },
    { package   => __PACKAGE__,
      name      => 'Baer',
      universal => 1,
      params    => 1,
      encodesub => sub {shift->put_baer(@_)},
      decodesub => sub {shift->get_baer(@_)}, },
    { package   => __PACKAGE__,
      name      => 'BoldiVigna',
      universal => 1,
      params    => 1,
      encodesub => sub {shift->put_boldivigna(@_)},
      decodesub => sub {shift->get_boldivigna(@_)}, },
    { package   => __PACKAGE__,
      name      => 'StartStop',
      universal => 1,
      params    => 1,
      encodesub => sub {shift->put_startstop([split('-',shift)], @_)},
      decodesub => sub {shift->get_startstop([split('-',shift)], @_)}, },
    { package   => __PACKAGE__,
      name      => 'StartStepStop',
      universal => 1,
      params    => 1,
      encodesub => sub {shift->put_startstepstop([split('-',shift)], @_)},
      decodesub => sub {shift->get_startstepstop([split('-',shift)], @_)}, },
   );
my %codeinfo;

sub add_code {
  my $rinfo = shift;
  die "add_code needs a hash ref" unless defined $rinfo && ref $rinfo eq 'HASH';
  foreach my $p (qw(package name universal params encodesub decodesub)) {
    die "invalid registration: missing $p" unless defined $$rinfo{$p};
  }
  my $name = lc $$rinfo{'name'};
  if (defined $codeinfo{$name}) {
    return 1 if $codeinfo{$name}{'package'} eq $$rinfo{'package'};
    die "module $$rinfo{'package'} trying to reuse code name '$name' already in 
use by $codeinfo{$name}{'package'}";
  }
  $codeinfo{$name} = $rinfo;
  1;
}

sub _init_codeinfo {
  if (scalar @_initinfo > 0) {
    foreach my $rinfo (@_initinfo) {
      add_code($rinfo);
    }
    @_initinfo = ();
  }
}

sub find_code {
  my $code = lc shift;

  _init_codeinfo if scalar @_initinfo > 0;
  return $codeinfo{$code};
}

sub code_is_supported {
  my $code = lc shift;
  my $param;  $param = $1 if $code =~ s/\((.+)\)$//;
  return defined find_code($code);
}

sub code_is_universal {
  my $code = lc shift;
  my $param;  $param = $1 if $code =~ s/\((.+)\)$//;
  my $inforef = find_code($code);
  if (!defined $inforef) {
    warn "code_is_universal: unknown code '$code'\n";
    return 0;
  }
  return $inforef->{'universal'};
}

# It would be nice to speed these up, but doing so isn't trivial.  I've added
# a couple shortcuts for Unary and Gamma, but it isn't a generic solution.
sub code_put {
  my $self = shift;
  my $code = lc shift;
  if    ($code eq 'unary' ) { return $self->put_unary(@_); }
  elsif ($code eq 'gamma' ) { return $self->put_gamma(@_); }
  my $param;  $param = $1 if $code =~ s/\((.+)\)$//;
  my $inforef = $codeinfo{$code};
  $inforef = find_code($code) unless defined $inforef;
  die "Unknown code $code" unless defined $inforef;
  my $sub = $inforef->{'encodesub'};
  die "No encoding sub for code $code!" unless defined $sub;
  if ($inforef->{'params'}) {
    die "Code $code needs a parameter" unless defined $param;
    return $sub->($self, $param, @_);
  } else {
    die "Code $code does not have parameters" if defined $param;
    return $sub->($self, @_);
  }
}

sub code_get {
  my $self = shift;
  my $code = lc shift;
  if    ($code eq 'unary' ) { return $self->get_unary(@_); }
  elsif ($code eq 'gamma' ) { return $self->get_gamma(@_); }
  my $param;  $param = $1 if $code =~ s/\((.+)\)$//;
  my $inforef = $codeinfo{$code};
  $inforef = find_code($code) unless defined $inforef;
  die "Unknown code $code" unless defined $inforef;
  my $sub = $inforef->{'decodesub'};
  die "No decoding sub for code $code!" unless defined $sub;
  if ($inforef->{'params'}) {
    die "Code $code needs a parameter" unless defined $param;
    return $sub->($self, $param, @_);
  } else {
    die "Code $code does not have parameters" if defined $param;
    return $sub->($self, @_);
  }
}


################################################################################
#
#                               CLASS METHODS
#
################################################################################

1;

__END__


# ABSTRACT: A bit stream class including integer coding methods.

=pod

=head1 NAME

Data::BitStream::XS - A bit stream class including integer coding methods

=head1 SYNOPSIS

  use Data::BitStream::XS;
  my $stream = Data::BitStream::XS->new;
  $stream->put_gamma($_) for (1 .. 20);
  $stream->rewind_for_read;
  my @values = $stream->get_gamma(-1);

See L<Data::BitStream> for more examples.

=head1 DESCRIPTION

An XS implementation providing read/write access to bit streams.  This includes
many integer coding methods as well as straightforward ways to implement new
codes.

Bit streams are often used in data compression and in embedded products where
memory is at a premium.

This code provides a nearly drop-in XS replacement for the L<Data::BitStream>
module.  If you do not need the ability to add custom codes, or the flexibility
of the Moose/Mouse system, you can use this directly.

The L<Data::BitStream> class will attempt to use this class if it is available.
Most operations will be 50-100 times faster, while not sacrificing any of its
flexibility, so it is highly recommended.

There is not a lot of extra speed gained by using the XS class directly, so
maximum portability and flexibility is had by using the L<Data::BitStream>
class and installing this module to get the speed.

=head1 METHODS

=head2 CLASS METHODS

=over 4

=item B< maxbits >

Returns the number of bits in a word, which is the largest allowed size of
the C<bits> argument to C<read> and C<write>.  This will be either 32 or 64.

=back

=head2 OBJECT METHODS (I<reading>)

These methods are only value while the stream is in reading state.

=over 4

=item B< rewind >

Moves the position to the stream beginning.

=item B< exhausted >

Returns true is the stream is at the end.  Rarely used.

=item B< read($bits [, 'readahead']) >

Reads C<$bits> from the stream and returns the value.
C<$bits> must be between C<1> and C<maxbits>.

The position is advanced unless the second argument is the string 'readahead'.

=item B< skip($bits) >

Advances the position C<$bits> bits.  Used in conjunction with C<readahead>.

=item B< read_string($bits) >

Reads C<$bits> bits from the stream and returns them as a binary string, such
as '0011011'.

=back

=head2 OBJECT METHODS (I<writing>)

These methods are only value while the stream is in writing state.

=over 4

=item B< write($bits, $value) >

Writes C<$value> to the stream using C<$bits> bits.  
C<$bits> must be between C<1> and C<maxbits>.

The length is increased by C<$bits> bits.
Regardless of the contents of C<$value>, exactly C<$bits> bits will be used.
If C<$value> has more non-zero bits than C<$bits>, the lower bits are written.
In other words, C<$value> will be masked before writing.

=item B< put_string(@strings) >

Takes one or more binary strings, such as '1001101', '001100', etc. and
writes them to the stream.  The number of bits used for each value is equal
to the string length.

=item B< put_stream($source_stream) >

Writes the contents of C<$source_stream> to the stream.  This is a helper
method that might be more efficient than doing it in one of the many other
possible ways.  The default implementation uses:

  $self->put_string( $source_stream->to_string );

=back

=head2 OBJECT METHODS (I<conversion>)

These methods may be called at any time, and will adjust the state of the
stream.

=over 4

=item B< to_string >

Returns the stream as a binary string, e.g. '00110101'.

=item B< to_raw >

Returns the stream as packed big-endian data.  This form is portable to
any other implementation on any architecture.

=item B< to_store >

Returns the stream as some scalar holding the data in some implementation
specific way.  This may be portable or not, but it can always be read by
the same implementation.  It might be more efficient than the raw format.

=item B< from_string($string) >

The stream will be set to the binary string C<$string>.

=item B< from_raw($packed [, $bits]) >

The stream is set to the packed big-endian vector C<$packed> which has
C<$bits> bits of data.  If C<$bits> is not present, then C<length($packed)>
will be used as the byte-length.  It is recommended that you include C<$bits>.

=item B< from_store($blob [, $bits]) >

Similar to C<from_raw>, but using the value returned by C<to_store>.

=back

=head2 OBJECT METHODS (I<other>)

=over 4

=item B< pos >

A read-only non-negative integer indicating the current position in a read
stream.  It is advanced by C<read>, C<get>, and C<skip> methods, as well
as changed by C<to>, C<from>, C<rewind>, and C<erase> methods.

=item B< len >

A read-only non-negative integer indicating the current length of the stream
in bits.  It is advanced by C<write> and C<put> methods, as well as changed
by C<from> and C<erase> methods.

=item B< writing >

A read-only boolean indicating whether the stream is open for writing or
reading.  Methods for read such as
C<read>, C<get>, C<skip>, C<rewind>, C<skip>, and C<exhausted>
are not allowed while writing.  Methods for write such as
C<write> and C<put>
are not allowed while reading.  

The C<write_open> and C<erase_for_write> methods will set writing to true.
The C<write_close> and C<rewind_for_read> methods will set writing to false.

The read/write distinction allows implementations more freedom in internal
caching of data.  For instance, they can gather writes into blocks.  It also
can be helpful in catching mistakes such as reading from a target stream.

=item B< erase >

Erases all the data, while the writing state is left unchanged.  The position
and length will both be 0 after this is finished.

=item B< write_open >

Changes the state to writing with no other API-visible changes.

=item B< write_close >

Changes the state to reading, and the position is set to the end of the
stream.  No other API-visible changes happen.

=item B< erase_for_write >

A helper function that performs C<erase> followed by C<write_open>.

=item B< rewind_for_read >

A helper function that performs C<write_close> followed by C<rewind>.

=back

=head2 OBJECT METHODS (I<coding>)

All coding methods are biased to 0.  This means values from 0 to 2^maxbits-1
(for universal codes) may be encoded, even if the original code as published
starts with 1.

All C<get_> methods take an optional count as the last argument.
If C<$count> is C<1> or not supplied, a single value will be read.
If C<$count> is positive, that many values will be read.
If C<$count> is negative, values are read until the end of the stream.

C<get_> methods called in list context this return a list of all values read.
Called in scalar context they return the last value read.

C<put_> methods take one or more values as input after any optional
parameters and write them to the stream.  All values must be non-negative
integers that do not exceed the maximum encodable value (~0 for universal
codes, parameter-specific for others).

=over 4

=item B< get_unary([$count]) >

=item B< put_unary(@values) >

Reads/writes one or more values from the stream in C<0000...1> unary coding.
Unary coding is only appropriate for relatively small numbers, as it uses
C<$value + 1> bits per value.

=item B< get_unary1([$count]) >

=item B< put_unary1(@values) >

Reads/writes one or more values from the stream in C<1111...0> unary coding.

=item B< get_binword($bits, [$count]) >

=item B< put_binword($bits, @values) >

Reads/writes one or more values from the stream as fixed-length binary
numbers, each using C<$bits> bits.

=item B< get_gamma([$count]) >

=item B< put_gamma(@values) >

Reads/writes one or more values from the stream in Elias Gamma coding.

=item B< get_delta([$count]) >

=item B< put_delta(@values) >

Reads/writes one or more values from the stream in Elias Delta coding.

=item B< get_omega([$count]) >

=item B< put_omega(@values) >

Reads/writes one or more values from the stream in Elias Omega coding.

=item B< get_levenstein([$count]) >

=item B< put_levenstein(@values) >

Reads/writes one or more values from the stream in Levenstein coding
(sometimes called Levenshtein or Левенште́йн coding).

=item B< get_evenrodeh([$count]) >

=item B< put_evenrodeh(@values) >

Reads/writes one or more values from the stream in Even-Rodeh coding.

=item B< get_fib([$count]) >

=item B< put_fib(@values) >

Reads/writes one or more values from the stream in Fibonacci coding.
Specifically, the order C<m=2> C1 codes of Fraenkel and Klein.

=item B< get_golomb($m [, $count]) >

=item B< put_golomb($m, @values) >

Reads/writes one or more values from the stream in Golomb coding.

=item B< get_golomb(sub { ... }, $m [, $count]) >

=item B< put_golomb(sub { ... }, $m, @values) >

Reads/writes one or more values from the stream in Golomb coding using the
supplied subroutine instead of unary coding, which can make them work with
large outliers.  For example to use Fibonacci coding for the base:

  $stream->put_golomb( sub {shift->put_fib(@_)}, $m, $value);

  $value = $stream->put_golomb( sub {shift->get_fib(@_)}, $m);

=item B< get_rice($k [, $count]) >

=item B< put_rice($k, @values) >

Reads/writes one or more values from the stream in Rice coding, which is
the time efficient case where C<m = 2^k>.

=item B< get_rice(sub { ... }, $k [, $count]) >

=item B< put_rice(sub { ... }, $k, @values) >

Reads/writes one or more values from the stream in Rice coding, which is
the time efficient case where C<m = 2^k>.

=item B< get_rice(sub { ... }, $k [, $count]) >

=item B< put_rice(sub { ... }, $k, @values) >

Reads/writes one or more values from the stream in Rice coding using the
supplied subroutine instead of unary coding, which can make them work with
large outliers.  For example to use Omega coding for the base:

  $stream->put_rice( sub {shift->put_omega(@_)}, $k, $value);

  $value = $stream->put_rice( sub {shift->get_omega(@_)}, $k);

=item B< get_gammagolomb($m [, $count]) >

=item B< put_gammagolomb($m, @values) >

Reads/writes one or more values from the stream in Golomb coding using
Elias Gamma codes for the base.  This is a convenience since they are common.

=item B< get_expgolomb($k [, $count]) >

=item B< put_expgolomb($k, @values) >

Reads/writes one or more values from the stream in Rice coding using
Elias Gamma codes for the base.  This is a convenience since they are common.

=item B< get_startstop(\@m [, $count]) >

=item B< put_startstop(\@m, @values) >

Reads/writes one or more values using Start/Stop codes.  The parameter is an
array reference which can be an anonymous array, for example:

  $stream->put_startstop( [0,3,2,0], @array );
  my @array2 = $stream->get_startstop( [0,3,2,0], -1);

=item B< get_startstepstop(\@m [, $count]) >

=item B< put_startstepstop(\@m, @values) >

Reads/writes one or more values using Start-Step-Stop codes.  The parameter
is an array reference which can be an anonymous array, for example:

  $stream->put_startstepstop( [3,2,9], @array );
  my @array3 = $stream->get_startstepstop( [3,2,9], -1);

=back

=head1 SEE ALSO

=over 4

=item L<Data::BitStream::Base>

=item L<Data::BitStream::WordVec>

=item L<Data::BitStream::Code::Gamma>

=item L<Data::BitStream::Code::Delta>

=item L<Data::BitStream::Code::Omega>

=item L<Data::BitStream::Code::Levenstein>

=item L<Data::BitStream::Code::EvenRodeh>

=item L<Data::BitStream::Code::Fibonacci>

=item L<Data::BitStream::Code::Golomb>

=item L<Data::BitStream::Code::Rice>

=item L<Data::BitStream::Code::GammaGolomb>

=item L<Data::BitStream::Code::ExponentialGolomb>

=item L<Data::BitStream::Code::StartStop>

=back

=head1 AUTHORS

Dana Jacobsen <dana@acm.org>

=head1 COPYRIGHT

Copyright 2011 by Dana Jacobsen <dana@acm.org>

This program is free software; you can redistribute it and/or modify it under the same terms as Perl itself.

=cut
