#!/usr/bin/perl
use warnings;
use strict;
use List::Util;

########################################################################
# Heavily simplified test runner for Zephyr qemu tests.
# + Only understands ztest tests (no yaml regexes)
# + Relies on a pre-built sanitycheck directory with pre-cooked qemu
#   commands
# + Guarantees that no process underneath this script should contend
#   with a qemu instance for CPU cycles.
# + Uses minimal CPU internally (mostly polling on filesystem stat
#   operations between 1-second sleeps) at the expense of a little
#   latency

my $TIMEOUT = 60;

########################################################################
# Pass a file containing a list of pre-built sanitycheck directories
# on the command line.

my @DIRS;
my $dirlist = shift or die;
open my $f, $dirlist or die;
while(<$f>) {
    chomp;
    push @DIRS, $_;
}
close $f;

########################################################################
# Compute runnable job count.  Use one fewer than the true number (not
# hyperthreaded count) of cores.  Should do this better so it supports
# non-HT CPUs without cutting performance in half.

my $JOBS = (`grep ^processor /proc/cpuinfo  | wc -l` / 2) - 1;
print "JOBS $JOBS\n";

########################################################################
# Generate a qemu command line for each test

my %cmds = ();
my @tests;
foreach my $d (@DIRS) {
    my $cmd = `cat $d/qemu-cmd-run` or next;

    # Need to clean up the command.  First unify whitespace
    $cmd =~ s/\s+/ /gs;

    # Scrub any (arbitrarily nested) unexpanded cmake variables that
    # we find, replacing them with a "."
    while($cmd =~ /\$\{[^{}]*\}/s) {
	$cmd =~ s/\$\{[^{}]*\}/./gs;
    }

    # Now replace "-pidfile xx" and "-serial yy" arguments.  Also
    # redirect the monitor to a file.
    $cmd =~ s/ -pidfile [^ ]+//;
    $cmd =~ s/ -serial [^ ]+//;
    $cmd .= " -pidfile $d/qemu-pid";
    $cmd .= " -serial file:$d/qemu-serial.out";
    $cmd .= " -monitor file:$d/qemu-monitor.out";

    # x86_64 uses a different qemu image for runtime than the linker
    # output.  Also sanitycheck partially populates directories for
    # skipped tests; don't include them if they didn't actually build
    # a qemu kernel image.
    if(-e "$d/zephyr-qemu.elf") {
        $cmd .= " -kernel $d/zephyr-qemu.elf";
    } elsif(-e "$d/zephyr/zephyr.elf") {
        $cmd .= " -kernel $d/zephyr/zephyr.elf";
    } else {
        print "WARNING: no ELF image found for $d, skipping\n";
        next;
    }

    push @tests, $d;
    $cmds{$d} = $cmd;
}

########################################################################
# Now run

# Randomize order to prevent clustering on platform
@tests = List::Util::shuffle(@tests);

my %started;
my $nrunning = 0;

while(@tests > 0 || $nrunning) {
    for my $t (keys %started) {
	if(time() - $started{$t} > $TIMEOUT) {
	    end_test("FAIL (timeout)", $t);
	} else {
	    # Check mtime on the output file, it it looks
	    # stable/unchanging, parse it.
	    next if ! -e "$t/qemu-serial.out";
	    my $since_output = time() - (stat("$t/qemu-serial.out"))[9];
	    if($since_output > 2) {
		my $out = `cat $t/qemu-serial.out`;
		if($out =~ /PROJECT EXECUTION (SUCCESSFUL|FAILED)/s) {
		    end_test($1 eq "FAILED" ? "FAIL (test)" : "PASS", $t);
		}
	    }
	}
    }
    
    while($nrunning < $JOBS && @tests > 0) {
	# Spawn one
	my $t = shift @tests;
	$started{$t} = time();

        $nrunning++;

        # x86_64 is SMP, which needs an extra job to handle the load.
        # Note that this isn't 100% correct, if there's only one slot
        # available we'll still spawn the process
        $nrunning++ if $t =~ /qemu_x86_64/;

	print "! $t\n";

	for my $f (qw(serial monitor stdout stderr)) {
	    unlink "$t/qemu-$f.out";
	}
	unlink "$t/qemu-pid";

	# Note that we capture stdout/err separately from -serial and
	# -monitor, becuase I still see stuff on the console that
	# isn't part of those streams...
	system("$cmds{$t} > $t/qemu-stdout.out 2> $t/qemu-stderr.out &");
    }

    sleep 1;
}

sub end_test {
    my ($msg, $t) = @_;
    if(-e "$t/qemu-pid") {
	kill 9, (`cat $t/qemu-pid` + 0);
    }
    delete $started{$t};
    $nrunning--;
    $nrunning-- if $t =~ /qemu_x86_64/; # These take an extra job!

    # FIXME: parse $t to remove directory path and split out
    # platform/test/case
    print "$msg $t\n";
}
