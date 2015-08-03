#!/usr/bin/perl
#
# FILE:		sha2test.pl
# AUTHOR:	Aaron D. Gifford - http://www.aarongifford.com/
# 
# Copyright (c) 2001, Aaron D. Gifford
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. Neither the name of the copyright holder nor the names of contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTOR(S) ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTOR(S) BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
# $Id: sha2test.pl,v 1.1 2001/11/08 00:02:37 adg Exp adg $
#

sub usage {
	my ($err) = shift(@_);

	print <<EOM;
Error:
	$err
Usage:
	$0 [<options>] [<test-vector-info-file> [<test-vector-info-file> ...]]

Options:
	-256	Use SHA-256 hashes during testing
	-384	Use SHA-384 hashes during testing
	-512	Use SHA-512 hashes during testing
	-ALL	Use all three hashes during testing
	-c256 <command-spec>	Specify a command to execute to generate a
				SHA-256 hash.  Be sure to include a '%'
				character which will be replaced by the
				test vector data filename containing the
				data to be hashed.  This command implies
				the -256 option.
	-c384 <command-spec>	Specify a command to execute to generate a
				SHA-384 hash.  See above.  Implies -384.
	-c512 <command-spec>	Specify a command to execute to generate a
				SHA-512 hash.  See above.  Implies -512.
	-cALL <command-spec>	Specify a command to execute that will
				generate all three hashes at once and output
				the data in hexadecimal.  See above for
				information about the <command-spec>.
				This option implies the -ALL option, and
				also overrides any other command options if
				present.

By default, this program expects to execute the command ./sha2 within the
current working directory to generate all hashes.  If no test vector
information files are specified, this program expects to read a series of
files ending in ".info" within a subdirectory of the current working
directory called "testvectors".

EOM
	exit(-1);
}

$c256 = $c384 = $c512 = $cALL = "";
$hashes = 0;
@FILES = ();

# Read all command-line options and files:
while ($opt = shift(@ARGV)) {
	if ($opt =~ s/^\-//) {
		if ($opt eq "256") {
			$hashes |= 1;
		} elsif ($opt eq "384") {
			$hashes |= 2;
		} elsif ($opt eq "512") {
			$hashes |= 4;
		} elsif ($opt =~ /^ALL$/i) {
			$hashes = 7;
		} elsif ($opt =~ /^c256$/i) {
			$hashes |= 1;
			$opt = $c256 = shift(@ARGV);
			$opt =~ s/\s+.*$//;
			if (!$c256 || $c256 !~ /\%/ || !-x $opt) {
				usage("Missing or invalid command specification for option -c256: $opt\n");
			}
		} elsif ($opt =~ /^c384$/i) {
			$hashes |= 2;
			$opt = $c384 = shift(@ARGV);
			$opt =~ s/\s+.*$//;
			if (!$c384 || $c384 !~ /\%/ || !-x $opt) {
				usage("Missing or invalid command specification for option -c384: $opt\n");
			}
		} elsif ($opt =~ /^c512$/i) {
			$hashes |= 4;
			$opt = $c512 = shift(@ARGV);
			$opt =~ s/\s+.*$//;
			if (!$c512 || $c512 !~ /\%/ || !-x $opt) {
				usage("Missing or invalid command specification for option -c512: $opt\n");
			}
		} elsif ($opt =~ /^cALL$/i) {
			$hashes = 7;
			$opt = $cALL = shift(@ARGV);
			$opt =~ s/\s+.*$//;
			if (!$cALL || $cALL !~ /\%/ || !-x $opt) {
				usage("Missing or invalid command specification for option -cALL: $opt\n");
			}
		} else {
			usage("Unknown/invalid option '$opt'\n");
		}
	} else {
		usage("Invalid, nonexistent, or unreadable file '$opt': $!\n") if (!-f $opt);
		push(@FILES, $opt);
	}
}

# Set up defaults:
if (!$cALL && !$c256 && !$c384 && !$c512) {
	$cALL = "./sha2 -ALL %";
	usage("Required ./sha2 binary executable not found.\n") if (!-x "./sha2");
}
$hashes = 7 if (!$hashes);

# Do some sanity checks:
usage("No command was supplied to generate SHA-256 hashes.\n") if ($hashes & 1 == 1 && !$cALL && !$c256);
usage("No command was supplied to generate SHA-384 hashes.\n") if ($hashes & 2 == 2 && !$cALL && !$c384);
usage("No command was supplied to generate SHA-512 hashes.\n") if ($hashes & 4 == 4 && !$cALL && !$c512);

# Default .info files:
if (scalar(@FILES) < 1) {
	opendir(DIR, "testvectors") || usage("Unable to scan directory 'testvectors' for vector information files: $!\n");
	@FILES = grep(/\.info$/, readdir(DIR));
	closedir(DIR);
	@FILES = map { s/^/testvectors\//; $_; } @FILES;
	@FILES = sort(@FILES);
}

# Now read in each test vector information file:
foreach $file (@FILES) {
	$dir = $file;
	if ($file !~ /\//) {
		$dir = "./";
	} else {
		$dir =~ s/\/[^\/]+$//;
		$dir .= "/";
	}
	open(FILE, "<" . $file) ||
		usage("Unable to open test vector information file '$file' for reading: $!\n");
	$vec = { desc => "", file => "", sha256 => "", sha384 => "", sha512 => "" };
	$data = $field = "";
	$line = 0;
	while(<FILE>) {
		$line++;
		s/\s*[\r\n]+$//;
		next if ($field && $field ne "DESCRIPTION" && !$_);
		if (/^(DESCRIPTION|FILE|SHA256|SHA384|SHA512):$/) {
			if ($field eq "DESCRIPTION") {
				$vec->{desc} = $data;
			} elsif ($field eq "FILE") {
				$data = $dir . $data if ($data !~ /^\//);
				$vec->{file} = $data;
			} elsif ($field eq "SHA256") {
				$vec->{sha256} = $data;
			} elsif ($field eq "SHA384") {
				$vec->{sha384} = $data;
			} elsif ($field eq "SHA512") {
				$vec->{sha512} = $data;
			}
			$data = "";
			$field = $1;
		} elsif ($field eq "DESCRIPTION") {
			s/^    //;
			$data .= $_ . "\n";
		} elsif ($field =~ /^SHA\d\d\d$/) {
			s/^\s+//;
			if (!/^([a-f0-9]{32}|[a-f0-9]{64})$/) {
				usage("Invalid SHA-256/384/512 test vector information " .
				      "file format at line $line of file '$file'\n");
			}
			$data .= $_;
		} elsif ($field eq "FILE") {
			s/^    //;
			$data .= $_;
		} else {
			usage("Invalid SHA-256/384/512 test vector information file " .
			      "format at line $line of file '$file'\n");
		}
	}
	if ($field eq "DESCRIPTION") {
		$data = $dir . $data if ($data !~ /^\//);
		$vec->{desc} = $data;
	} elsif ($field eq "FILE") {
		$vec->{file} = $data;
	} elsif ($field eq "SHA256") {
		$vec->{sha256} = $data;
	} elsif ($field eq "SHA384") {
		$vec->{sha384} = $data;
	} elsif ($field eq "SHA512") {
		$vec->{sha512} = $data;
	} else {
		usage("Invalid SHA-256/384/512 test vector information file " .
		      "format.  Missing required fields in file '$file'\n");
	}

	# Sanity check all entries:
	if (!$vec->{desc}) {
		usage("Invalid SHA-256/384/512 test vector information file " .
		      "format.  Missing required DESCRIPTION field in file '$file'\n");
	}
	if (!$vec->{file}) {
		usage("Invalid SHA-256/384/512 test vector information file " .
		      "format.  Missing required FILE field in file '$file'\n");
	}
	if (! -f $vec->{file}) {
		usage("The test vector data file (field FILE) name " .
		      "'$vec->{file}' is not a readable file.  Check the FILE filed in " .
		      "file '$file'.\n");
	}
	if (!($vec->{sha256} || $vec->{sha384} || $vec->{sha512})) {
		usage("Invalid SHA-256/384/512 test vector information file " .
		      "format.  There must be at least one SHA256, SHA384, or SHA512 " .
		      "field specified in file '$file'.\n");
	}
	if ($vec->{sha256} !~ /^(|[a-f0-9]{64})$/) {
		usage("Invalid SHA-256/384/512 test vector information file " .
		      "format.  The SHA256 field is invalid in file '$file'.\n");
	}
	if ($vec->{sha384} !~ /^(|[a-f0-9]{96})$/) {
		usage("Invalid SHA-256/384/512 test vector information file " .
		      "format.  The SHA384 field is invalid in file '$file'.\n");
	}
	if ($vec->{sha512} !~ /^(|[a-f0-9]{128})$/) {
		usage("Invalid SHA-256/384/512 test vector information file " .
		      "format.  The SHA512 field is invalid in file '$file'.\n");
	}
	close(FILE);
	if ($hashes & (($vec->{sha256} ? 1 : 0) | ($vec->{sha384} ? 2 : 0) | ($vec->{sha512} ? 4 : 0))) {
		push(@VECTORS, $vec);
	}
}

usage("There were no test vectors for the specified hash(es) in any of the test vector information files you specified.\n") if (scalar(@VECTORS) < 1);

$num = $errors = $error256 = $error384 = $error512 = $tests = $test256 = $test384 = $test512 = 0;
foreach $vec (@VECTORS) {
	$num++;
	print "TEST VECTOR #$num:\n";
	print "\t" . join("\n\t", split(/\n/, $vec->{desc})) . "\n";
	print "VECTOR DATA FILE:\n\t$vec->{file}\n";
	$sha256 = $sha384 = $sha512 = "";
	if ($cALL) {
		$prog = $cALL;
		$prog =~ s/\%/'$vec->{file}'/g;
		@SHA = grep(/[a-fA-f0-9]{64,128}/, split(/\n/, `$prog`));
		($sha256) = grep(/(^[a-fA-F0-9]{64}$|^[a-fA-F0-9]{64}[^a-fA-F0-9]|[^a-fA-F0-9][a-fA-F0-9]{64}$|[^a-fA-F0-9][a-fA-F0-9]{64}[^a-fA-F0-9])/, @SHA);
		($sha384) = grep(/(^[a-fA-F0-9]{96}$|^[a-fA-F0-9]{96}[^a-fA-F0-9]|[^a-fA-F0-9][a-fA-F0-9]{96}$|[^a-fA-F0-9][a-fA-F0-9]{96}[^a-fA-F0-9])/, @SHA);
		($sha512) = grep(/(^[a-fA-F0-9]{128}$|^[a-fA-F0-9]{128}[^a-fA-F0-9]|[^a-fA-F0-9][a-fA-F0-9]{128}$|[^a-fA-F0-9][a-fA-F0-9]{128}[^a-fA-F0-9])/, @SHA);
	} else {
		if ($c256) {
			$prog = $c256;
			$prog =~ s/\%/'$vec->{file}'/g;
			@SHA = grep(/[a-fA-f0-9]{64,128}/, split(/\n/, `$prog`));
			($sha256) = grep(/(^[a-fA-F0-9]{64}$|^[a-fA-F0-9]{64}[^a-fA-F0-9]|[^a-fA-F0-9][a-fA-F0-9]{64}$|[^a-fA-F0-9][a-fA-F0-9]{64}[^a-fA-F0-9])/, @SHA);
		}
		if ($c384) {
			$prog = $c384;
			$prog =~ s/\%/'$vec->{file}'/g;
			@SHA = grep(/[a-fA-f0-9]{64,128}/, split(/\n/, `$prog`));
			($sha384) = grep(/(^[a-fA-F0-9]{96}$|^[a-fA-F0-9]{96}[^a-fA-F0-9]|[^a-fA-F0-9][a-fA-F0-9]{96}$|[^a-fA-F0-9][a-fA-F0-9]{96}[^a-fA-F0-9])/, @SHA);
		}
		if ($c512) {
			$prog = $c512;
			$prog =~ s/\%/'$vec->{file}'/g;
			@SHA = grep(/[a-fA-f0-9]{64,128}/, split(/\n/, `$prog`));
			($sha512) = grep(/(^[a-fA-F0-9]{128}$|^[a-fA-F0-9]{128}[^a-fA-F0-9]|[^a-fA-F0-9][a-fA-F0-9]{128}$|[^a-fA-F0-9][a-fA-F0-9]{128}[^a-fA-F0-9])/, @SHA);
		}
	}
	usage("Unable to generate any hashes for file '$vec->{file}'!\n") if (!$sha256 && !$sha384 && $sha512);
	$sha256 =~ tr/A-F/a-f/;
	$sha384 =~ tr/A-F/a-f/;
	$sha512 =~ tr/A-F/a-f/;
	$sha256 =~ s/^.*([a-f0-9]{64}).*$/$1/;
	$sha384 =~ s/^.*([a-f0-9]{96}).*$/$1/;
	$sha512 =~ s/^.*([a-f0-9]{128}).*$/$1/;

	if ($sha256 && $hashes & 1 == 1) {
		if ($vec->{sha256} eq $sha256) {
			print "SHA256 MATCHES:\n\t$sha256\n"
		} else {
			print "SHA256 DOES NOT MATCH:\n\tEXPECTED:\n\t\t$vec->{sha256}\n" .
			      "\tGOT:\n\t\t$sha256\n\n";
			$error256++;
		}
		$test256++;
	}
	if ($sha384 && $hashes & 2 == 2) {
		if ($vec->{sha384} eq $sha384) {
			print "SHA384 MATCHES:\n\t" . substr($sha384, 0, 64) . "\n\t" .
			      substr($sha384, -32) . "\n";
		} else {
			print "SHA384 DOES NOT MATCH:\n\tEXPECTED:\n\t\t" .
			      substr($vec->{sha384}, 0, 64) . "\n\t\t" .
			      substr($vec->{sha384}, -32) . "\n\tGOT:\n\t\t" .
			      substr($sha384, 0, 64) . "\n\t\t" . substr($sha384, -32) . "\n\n";
			$error384++;
		}
		$test384++;
	}
	if ($sha512 && $hashes & 4 == 4) {
		if ($vec->{sha512} eq $sha512) {
			print "SHA512 MATCHES:\n\t" . substr($sha512, 0, 64) . "\n\t" .
			      substr($sha512, -64) . "\n";
		} else {
			print "SHA512 DOES NOT MATCH:\n\tEXPECTED:\n\t\t" .
			      substr($vec->{sha512}, 0, 64) . "\n\t\t" .
			      substr($vec->{sha512}, -32) . "\n\tGOT:\n\t\t" .
			      substr($sha512, 0, 64) . "\n\t\t" . substr($sha512, -64) . "\n\n";
			$error512++;
		}
		$test512++;
	}
}

$errors = $error256 + $error384 + $error512;
$tests = $test256 + $test384 + $test512;
print "\n\n===== RESULTS ($num VECTOR DATA FILES HASHED) =====\n\n";
print "HASH TYPE\tNO. OF TESTS\tPASSED\tFAILED\n";
print "---------\t------------\t------\t------\n";
if ($test256) {
	$pass = $test256 - $error256;
	print "SHA-256\t\t".substr("           $test256", -12)."\t".substr("     $pass", -6)."\t".substr("     $error256", -6)."\n";
}
if ($test384) {
	$pass = $test384 - $error384;
	print "SHA-384\t\t".substr("           $test384", -12)."\t".substr("     $pass", -6)."\t".substr("     $error384", -6)."\n";
}
if ($test512) {
	$pass = $test512 - $error512;
	print "SHA-512\t\t".substr("           $test512", -12)."\t".substr("     $pass", -6)."\t".substr("     $error512", -6)."\n";
}
print "----------------------------------------------\n";
$pass = $tests - $errors;
print "TOTAL:          ".substr("           $tests", -12)."\t".substr("     $pass", -6)."\t".substr("     $errors", -6)."\n\n";
print "NO ERRORS!  ALL TESTS WERE SUCCESSFUL!\n\n" if (!$errors);

