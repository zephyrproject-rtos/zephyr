#!/usr/bin/perl
use warnings;
use strict;

# Linker address generation validity checker.  By default, GNU ld is
# broken when faced with sections where the load address (i.e. the
# spot in the XIP program binary where initialized data lives) differs
# from the virtual address (i.e. the location in RAM where that data
# will live at runtime.  We need to be sure we're using the
# ALIGN_WITH_INPUT feature correctly everywhere, which is hard --
# especially so given that many of these bugs are semi-invisible at
# runtime (most initialized data is still a bunch of zeros and often
# "works" even if it's wrong).
#
# This quick test just checks the offsets between sequential segments
# with separate VMA/LMA addresses and verifies that the size deltas
# are identical.
#
# Note that this is assuming that the address generation is always
# in-order and that there is only one "ROM" LMA block.  It's possible
# to write a valid linker script that will fail this script, but we
# don't have such a use case and one isn't forseen.

# Skip the header stuff
while(<>) { last if /Linker script and memory map/; }


my ($last_sec, $last_vma, $last_lma);
while(<>) {
	next if ! /^([a-zA-Z0-9_\.]+) \s+  # name
		    (0x[0-9a-f]+)     \s+  # addr
		    (0x[0-9a-f]+)     \s+  # size
		  /x;

	my ($sec, $vma, $sz) = ($1, $2, $3);

	my $lma = "";
	if(/load address (0x[0-9a-f]+)/) {
		$lma = $1;
	} else {
		$last_sec = undef;
		next;
	}

	$vma = eval $vma;
	$lma = eval $lma;

	if(defined $last_sec) {
		my $dv = $vma - $last_vma;
		my $dl = $lma - $last_lma;
		if($dv != $dl) {
			print STDERR
			    "ERROR: section $last_sec is $dv bytes "
			    . "in the virtual/runtime address space, "
			    . "but only $dl in the loaded/XIP section!\n";
			exit 1;
		}
	}

	$last_sec = $sec;
	$last_vma = $vma;
	$last_lma = $lma;
}
