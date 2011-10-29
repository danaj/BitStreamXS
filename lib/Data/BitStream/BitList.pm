package Data::BitStream::BitList;
use strict;
use warnings;

use parent qw( Exporter );
our @EXPORT_OK = qw();

our $VERSION;
BEGIN {
  $VERSION = '0.01';
  eval {
    require XSLoader;
    XSLoader::load(__PACKAGE__, $VERSION);
    1;
  } or do {
    warn "Using PP BitList";
    die "Not implemented";
  }
}

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
  return 0 unless defined $source && $source->can('to_string');

  # in an implementation, you could check if ref $source eq __PACKAGE__
  # and do something special.

  $self->put_string($source->to_string);
  1;
}

1;
