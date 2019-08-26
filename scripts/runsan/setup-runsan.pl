#!/usr/bin/perl
use warnings;
use strict;

#
# Copy built sanitycheck trees from $ZEPHYR_BASE to the current
# directory and emit a directory list for use by runsan.pl
#

my $BASE = $ENV{ZEPHYR_BASE} or die "No ZEPHYR_BASE set\n";

my $tlist = `find $BASE -name qemu-cmd-run`;

my @tests = split /\n/, $tlist;

die "Couldn't find instrumented tests in $BASE\n" if !@tests;

foreach my $t (@tests) {
    $t =~ s/\/qemu-cmd-run$//;
}

# Add multiples (i.e. duplicate test runs if needed) to saturate the
# CPUs.  Leave one physical CPU free to ensure that routine host tasks
# don't interfere.
my $JOBS = (`grep ^processor /proc/cpuinfo  | wc -l` / 2) - 1;

my $ntests = 0 + @tests;
my $multiples = 1;
while(1) {
    my $tot = $multiples * $ntests;
    my $waste = $tot - $JOBS * int($tot / $JOBS);
    last if $waste / $tot < 0.05;
    $multiples++;
}

$tlist = "";
for my $t (@tests) {
    my $tmp = $t;
    $tmp =~ s/^$BASE\/sanity-out\///;
    $tlist .= " $tmp";
}

for(my $i=1; $i<=$multiples; $i++) {
    mkdir "$i" or die;
    system("(cd $BASE/sanity-out; tar cf - $tlist)"
	   . "| (cd $i; tar xf -)") == 0 or die;
}

system("find . -name qemu-cmd-run | xargs dirname > dirlist") == 0 or die;
