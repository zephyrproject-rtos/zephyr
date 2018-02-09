#!/usr/bin/perl -l
## Copyright (C) 2017 Intel Corporation
##
## Permission is hereby granted, free of charge, to any person obtaining a copy
## of this software and associated documentation files (the "Software"), to deal
## in the Software without restriction, including without limitation the rights
## to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
## copies of the Software, and to permit persons to whom the Software is
## furnished to do so, subject to the following conditions:
##
## The above copyright notice and this permission notice shall be included in
## all copies or substantial portions of the Software.
##
## THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
## IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
## FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
## AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
## LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
## OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
## THE SOFTWARE.
##
use strict;
my $fname = shift @ARGV
    or die("Usage: parsetags.pl tags.txt");
open TAGS, "<", $fname
    or die("Cannot open $fname: $!");

my %typedescriptions = (
    "Integer" => "integer",
    "ByteString" => "byte string",
    "TextString" => "UTF-8 text string",
    "Array" => "array",
    "Map" => "map",
    "Tag" => "tag",     # shouldn't happen
    "Simple" => "any simple type",
    "Boolean" => "boolean",
    "Null" => "null",
    "Undefined" => "undefined",
    "HalfFloat" => "IEEE 754 half-precision floating point",
    "Float" => "IEEE 754 single-precision floating point",
    "Double" => "IEEE 754 double-precision floating point"
);

my %tags;
while (<TAGS>) {
    s/\s*#.*$//;
    next if /^$/;
    chomp;

    die("Could not parse line \"$_\"")
        unless /^(\d+);(\w+);([\w,]*);(.*)$/;
    $tags{$1}{id} = $2;
    $tags{$1}{semantic} = $4;
    my @types = split(',', $3);
    $tags{$1}{types} = \@types;
}
close TAGS or die;

my @tagnumbers = sort { $a <=> $b } keys %tags;

print "==== HTML listing ====";
print "<table>\n  <tr>\n    <th>Tag</th>\n    <th>Data Item</th>\n    <th>Semantics</th>\n  </tr>";
for my $n (@tagnumbers) {
    print "  <tr>";
    print "    <td>$n</td>";

    my @types = @{$tags{$n}{types}};
    @types = map { $typedescriptions{$_}; } @types;
    unshift @types, "any"
        if (scalar @types == 0);
    printf "    <td>%s</td>\n", join(', ', @types);
    printf "    <td>%s</td>\n", $tags{$n}{semantic};
    print "  </td>";
}
print "</table>";

print "\n==== enum listing for cbor.h ====\n";
printf "typedef enum CborKnownTags {";
my $comma = "";
for my $n (@tagnumbers) {
    printf "%s\n    Cbor%sTag%s = %d", $comma,
        $tags{$n}{id},
        ' ' x (23 - length($tags{$n}{id})),
        $n;
    $comma = ",";
}
print "\n} CborKnownTags;";
print "\n/* #define the constants so we can check with #ifdef */";
for my $n (@tagnumbers) {
    printf "#define Cbor%sTag Cbor%sTag\n", $tags{$n}{id}, $tags{$n}{id};
}

print "\n==== search table ====\n";
print "struct KnownTagData { uint32_t tag; uint32_t types; };";
printf "static const struct KnownTagData knownTagData[] = {";
$comma = "";
for my $n (@tagnumbers) {
    my @types = @{$tags{$n}{types}};

    my $typemask;
    my $shift = 0;
    for my $type (@types) {
        die("Too many match types for tag $n") if $shift == 32;
        my $actualtype = "Cbor${type}Type";
        $actualtype = "($actualtype+1)" if $type eq "Integer";
        $typemask .= " | " if $typemask ne "";
        $typemask .= "((uint8_t)$actualtype << $shift)" if $shift;
        $typemask .= "(uint8_t)$actualtype" unless $shift;
        $shift += 8;
    }
    $typemask = "0U" if $typemask eq "";

    printf "%s\n    { %d, %s }", $comma, $n, $typemask;
    $comma = ",";
}
print "\n};";
