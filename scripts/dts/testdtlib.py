#!/usr/bin/env python3

# Copyright (c) 2019, Nordic Semiconductor
# SPDX-License-Identifier: BSD-3-Clause

import os
import shutil
import sys

import dtlib

# Test suite for dtlib.py. Run it directly as an executable, in this directory:
#
#   $ ./testdtlib.py

# TODO: Factor out common code from error tests


def run():
    """
    Runs all dtlib tests. Immediately exits with status 1 and a message on
    stderr on test suite failures.
    """
    # Note: All code is within this function, even though some triple-quoted
    # strings are unindented to make things less awkward

    def fail(msg):
        sys.exit("test failed: {}".format(msg))

    def parse(dts, include_path=()):
        open(".tmp.dts", "w").write(dts)
        return dtlib.DT(".tmp.dts", include_path)

    def verify_parse(dts, expected, include_path=()):
        # The [1:] is so that the first line can be put on a separate line
        # after """
        dt = parse(dts[1:], include_path)

        actual = str(dt)
        expected = expected[1:-1]
        if actual != expected:
            fail("expected '{}' to parse as '{}', parsed as '{}'"
                 .format(dts, expected, actual))

        return dt

    def verify_error(dts, msg):
        prefix = "expected '{}' to generate the error '{}', generated" \
                 .format(dts, msg)
        try:
            parse(dts[1:])
            fail(prefix + " no error")
        except dtlib.DTError as e:
            if str(e) != msg:
                fail("{} the error '{}'".format(prefix, e))
        except Exception as e:
            fail("{} the non-DTError '{}'".format(prefix, e))

    # These might already exist from failed earlier runs

    try:
        os.mkdir(".tmp")
    except OSError:
        pass

    try:
        os.mkdir(".tmp2")
    except OSError:
        pass

    #
    # Test cell parsing
    #

    verify_parse("""
/dts-v1/;

/ {
	a;
	b = < >;
	c = [ ];
	d = < 10 20 >;
	e = < 0U 1L 2UL 3LL 4ULL >;
	f = < 0x10 0x20 >;
	g = < 010 020 >;
	h = /bits/ 8 < 0x10 0x20 (-1) >;
	i = /bits/ 16 < 0x10 0x20 (-1) >;
	j = /bits/ 32 < 0x10 0x20 (-1) >;
	k = /bits/ 64 < 0x10 0x20 (-1) >;
	l = < 'a' 'b' 'c' >;
};
""",
"""
/dts-v1/;

/ {
	a;
	b;
	c;
	d = < 0xa 0x14 >;
	e = < 0x0 0x1 0x2 0x3 0x4 >;
	f = < 0x10 0x20 >;
	g = < 0x8 0x10 >;
	h = [ 10 20 FF ];
	i = /bits/ 16 < 0x10 0x20 0xffff >;
	j = < 0x10 0x20 0xffffffff >;
	k = /bits/ 64 < 0x10 0x20 0xffffffffffffffff >;
	l = < 0x61 0x62 0x63 >;
};
""")

    verify_error("""
/dts-v1/;

/ {
	a = /bits/ 16 < 0x10000 >;
};
""",
".tmp.dts:4 (column 18): parse error: 65536 does not fit in 16 bits")

    verify_error("""
/dts-v1/;

/ {
	a = < 0x100000000 >;
};
""",
".tmp.dts:4 (column 8): parse error: 4294967296 does not fit in 32 bits")

    verify_error("""
/dts-v1/;

/ {
	a = /bits/ 128 < 0 >;
};
""",
".tmp.dts:4 (column 13): parse error: expected 8, 16, 32, or 64")

    #
    # Test bytes parsing
    #

    verify_parse("""
/dts-v1/;

/ {
	a = [ ];
	b = [ 12 34 ];
	c = [ 1234 ];
};
""",
"""
/dts-v1/;

/ {
	a;
	b = [ 12 34 ];
	c = [ 12 34 ];
};
""")

    verify_error("""
/dts-v1/;

/ {
	a = [ 123 ];
};
""",
".tmp.dts:4 (column 10): parse error: expected two-digit byte or ']'")

    #
    # Test string parsing
    #

    verify_parse(r"""
/dts-v1/;

/ {
	a = "";
	b = "ABC";
	c = "\\\"\xab\377\a\b\t\n\v\f\r";
};
""",
r"""
/dts-v1/;

/ {
	a = "";
	b = "ABC";
	c = "\\\"\xab\xff\a\b\t\n\v\f\r";
};
""")

    verify_error(r"""
/dts-v1/;

/ {
	a = "\400";
};
""",
".tmp.dts:4 (column 6): parse error: octal escape out of range (> 255)")

    #
    # Test character literal parsing
    #

    verify_parse(r"""
/dts-v1/;

/ {
	a = < '\'' >;
	b = < '\x12' >;
};
""",
"""
/dts-v1/;

/ {
	a = < 0x27 >;
	b = < 0x12 >;
};
""")

    verify_error("""
/dts-v1/;

/ {
	// Character literals are not allowed at the top level
	a = 'x';
};
""",
".tmp.dts:5 (column 6): parse error: malformed value")

    verify_error("""
/dts-v1/;

/ {
	a = < '' >;
};
""",
".tmp.dts:4 (column 7): parse error: character literals must be length 1")

    verify_error("""
/dts-v1/;

/ {
	a = < '12' >;
};
""",
".tmp.dts:4 (column 7): parse error: character literals must be length 1")

    #
    # Test /incbin/
    #

    open(".tmp.bin", "wb").write(b"\00\01\02\03")

    verify_parse("""
/dts-v1/;

/ {
	a = /incbin/ (".tmp.bin");
	b = /incbin/ (".tmp.bin", 1, 1);
	c = /incbin/ (".tmp.bin", 1, 2);
};
""",
"""
/dts-v1/;

/ {
	a = [ 00 01 02 03 ];
	b = [ 01 ];
	c = [ 01 02 ];
};
""")

    open(".tmp/in_subdir", "wb").write(b"\00\01\02")

    verify_parse("""
/dts-v1/;

/ {
	a = /incbin/ ("in_subdir");
};
""",
"""
/dts-v1/;

/ {
	a = [ 00 01 02 ];
};
""",
    include_path=(".tmp",))

    verify_error(r"""
/dts-v1/;

/ {
	a = /incbin/ ("missing");
};
""",
".tmp.dts:4 (column 25): parse error: 'missing' could not be found")

    #
    # Test node merging
    #

    verify_parse("""
/dts-v1/;

/ {
	l1: l2: l1: foo {
		foo1 = [ 01 ];
		l4: l5: bar {
			bar1 = [ 01 ];
		};
	};
};

l3: &l1 {
	foo2 = [ 02 ];
	l6: l7: bar {
		bar2 = [ 02 ];
	};
};

&l3 {
	foo3 = [ 03 ];
};

&{/foo} {
	foo4 = [ 04 ];
};

&{/foo/bar} {
	bar3 = [ 03 ];
	l8: baz {};
};

/ {
};

/ {
	top = [ 01 ];
};
""",
"""
/dts-v1/;

/ {
	top = [ 01 ];
	l1: l2: l3: foo {
		foo1 = [ 01 ];
		foo2 = [ 02 ];
		foo3 = [ 03 ];
		foo4 = [ 04 ];
		l4: l5: l6: l7: bar {
			bar1 = [ 01 ];
			bar2 = [ 02 ];
			bar3 = [ 03 ];
			l8: baz {
			};
		};
	};
};
""")

    verify_error("""
/dts-v1/;

/ {
};

&missing {
};
""",
".tmp.dts:6 (column 1): parse error: undefined node label 'missing'")

    verify_error("""
/dts-v1/;

/ {
};

&{foo} {
};
""",
".tmp.dts:6 (column 1): parse error: node path 'foo' does not start with '/'")

    verify_error("""
/dts-v1/;

/ {
};

&{/foo} {
};
""",
".tmp.dts:6 (column 1): parse error: component 'foo' in path '/foo' does not exist")

    #
    # Test property labels
    #

    def verify_label2prop(label, expected):
        actual = dt.label2prop[label].name
        if actual != expected:
            fail("expected label '{}' to map to prop '{}', mapped to prop '{}'"
                 .format(label, expected, actual))

    dt = verify_parse("""
/dts-v1/;

/ {
	a;
	b;
	l2: c;
	l4: l5: l5: l4: d = < 0 >;
};

/ {
	l1: b;
	l3: c;
	l6: d;
};
""",
"""
/dts-v1/;

/ {
	a;
	l1: b;
	l2: l3: c;
	l4: l5: l6: d = < 0x0 >;
};
""")

    verify_label2prop("l1", "b")
    verify_label2prop("l2", "c")
    verify_label2prop("l3", "c")
    verify_label2prop("l4", "d")
    verify_label2prop("l5", "d")
    verify_label2prop("l6", "d")

    #
    # Test offset labels
    #

    def verify_label2offset(label, expected_prop, expected_offset):
        actual_prop, actual_offset = dt.label2prop_offset[label]
        actual_prop = actual_prop.name
        if (actual_prop, actual_offset) != (expected_prop, expected_offset):
            fail("expected label '{}' to map to offset {} on prop '{}', "
                 "mapped to offset {} on prop '{}'"
                 .format(label, expected_offset, expected_prop, actual_offset,
                         actual_prop))

    dt = verify_parse("""
/dts-v1/;

/ {
	a = l01: l02: < l03: &node l04: l05: 2 l06: >,
            l07: l08: [ l09: 03 l10: l11: 04 l12: l13: ] l14:, "A";

	b = < 0 > l23: l24:;

	node: node {
	};
};
""",
"""
/dts-v1/;

/ {
	a = l01: l02: < l03: &node l04: l05: 0x2 l06: l07: l08: >, [ l09: 03 l10: l11: 04 l12: l13: l14: ], "A";
	b = < 0x0 l23: l24: >;
	node: node {
		phandle = < 0x1 >;
	};
};
""")

    verify_label2offset("l01", "a", 0)
    verify_label2offset("l02", "a", 0)
    verify_label2offset("l04", "a", 4)
    verify_label2offset("l05", "a", 4)
    verify_label2offset("l06", "a", 8)
    verify_label2offset("l09", "a", 8)
    verify_label2offset("l10", "a", 9)

    verify_label2offset("l23", "b", 4)
    verify_label2offset("l24", "b", 4)

    #
    # Test unit_addr
    #

    def verify_unit_addr(path, expected):
        node = dt.get_node(path)
        if node.unit_addr != expected:
            fail("expected {!r} to have unit_addr '{}', was '{}'"
                 .format(node, expected, node.unit_addr))

    dt = verify_parse("""
/dts-v1/;

/ {
	no-unit-addr {
	};

	unit-addr@ABC {
	};

	unit-addr-non-numeric@foo-bar {
	};
};
""",
"""
/dts-v1/;

/ {
	no-unit-addr {
	};
	unit-addr@ABC {
	};
	unit-addr-non-numeric@foo-bar {
	};
};
""")

    verify_unit_addr("/no-unit-addr", "")
    verify_unit_addr("/unit-addr@ABC", "ABC")
    verify_unit_addr("/unit-addr-non-numeric@foo-bar", "foo-bar")

    #
    # Test node path references
    #

    verify_parse("""
/dts-v1/;

/ {
	a = &label;
	b = [ 01 ], &label;
	c = [ 01 ], &label, <2>;
	d = &{/abc};
	label: abc {
		e = &label;
		f = &{/abc};
	};
};
""",
"""
/dts-v1/;

/ {
	a = &label;
	b = [ 01 ], &label;
	c = [ 01 ], &label, < 0x2 >;
	d = &{/abc};
	label: abc {
		e = &label;
		f = &{/abc};
	};
};
""")

    verify_error("""
/dts-v1/;

/ {
	sub {
		x = &missing;
	};
};
""",
"/sub: undefined node label 'missing'")

    verify_error("""
/dts-v1/;

/ {
	sub {
		x = &{/sub/missing};
	};
};
""",
"/sub: component 'missing' in path '/sub/missing' does not exist")

    #
    # Test phandles
    #

    verify_parse("""
/dts-v1/;

/ {
	x = < &a &{/b} &c >;

	dummy1 {
		phandle = < 1 >;
	};

	dummy2 {
		phandle = < 3 >;
	};

	a: a {
	};

	b {
	};

	c: c {
		phandle = < 0xFF >;
	};
};
""",
"""
/dts-v1/;

/ {
	x = < &a &{/b} &c >;
	dummy1 {
		phandle = < 0x1 >;
	};
	dummy2 {
		phandle = < 0x3 >;
	};
	a: a {
		phandle = < 0x2 >;
	};
	b {
		phandle = < 0x4 >;
	};
	c: c {
		phandle = < 0xff >;
	};
};
""")

    # Check that a node can be assigned a phandle to itself. This just forces a
    # phandle to be allocated on it. The C tools support this too.
    verify_parse("""
/dts-v1/;

/ {
	dummy {
		phandle = < 1 >;
	};

	a {
                foo: phandle = < &{/a} >;
	};

	label: b {
                bar: phandle = < &label >;
	};
};
""",
"""
/dts-v1/;

/ {
	dummy {
		phandle = < 0x1 >;
	};
	a {
		foo: phandle = < &{/a} >;
	};
	label: b {
		bar: phandle = < &label >;
	};
};
""")

    verify_error("""
/dts-v1/;

/ {
	sub {
		x = < &missing >;
	};
};
""",
"/sub: undefined node label 'missing'")

    verify_error("""
/dts-v1/;

/ {
	a: sub {
		x = /bits/ 16 < &a >;
	};
};
""",
".tmp.dts:5 (column 19): parse error: phandle references are only allowed in arrays with 32-bit elements")

    verify_error("""
/dts-v1/;

/ {
	foo {
		phandle = [ 00 ];
	};
};
""",
"/foo: bad phandle length (1), expected 4 bytes")

    verify_error("""
/dts-v1/;

/ {
	foo {
		phandle = < 0 >;
	};
};
""",
"/foo: bad value 0x00000000 for phandle")

    verify_error("""
/dts-v1/;

/ {
	foo {
		phandle = < (-1) >;
	};
};
""",
"/foo: bad value 0xffffffff for phandle")

    verify_error("""
/dts-v1/;

/ {
	foo {
		phandle = < 17 >;
	};

	bar {
		phandle = < 17 >;
	};
};
""",
"/bar: duplicated phandle 0x11 (seen before at /foo)")

    verify_error("""
/dts-v1/;

/ {
	foo {
		phandle = < &{/bar} >;
	};

	bar {
	};
};
""",
"/foo: phandle refers to another node")

    # Test phandle2node

    def verify_phandle2node(prop, offset, expected_name):
        phandle = dtlib.to_num(dt.root.props[prop].value[offset:offset + 4])
        actual_name = dt.phandle2node[phandle].name

        if actual_name != expected_name:
            fail("expected {} to be a phandle for {}, was a phandle for {}"
                 .format(prop, expected_name, actual_name))

    dt = parse("""
/dts-v1/;

/ {
	phandle_ = < &{/node1} 0 1 >;
	phandles = < 0 &{/node2} 1 &{/node3} >;

	node1 {
		phandle = < 123 >;
	};

	node2 {
	};

	node3 {
	};
};
""")

    verify_phandle2node("phandle_", 0, "node1")
    verify_phandle2node("phandles", 4, "node2")
    verify_phandle2node("phandles", 12, "node3")

    #
    # Test mixed value type assignments
    #

    verify_parse("""
/dts-v1/;

/ {
	x = /bits/ 8 < 0xFF 0xFF >,
	    &abc,
	    < 0xFF &abc 0xFF &abc >,
	    &abc,
	    [ FF FF ],
	    "abc";

	abc: abc {
	};
};
""",
"""
/dts-v1/;

/ {
	x = [ FF FF ], &abc, < 0xff &abc 0xff &abc >, &abc, [ FF FF ], "abc";
	abc: abc {
		phandle = < 0x1 >;
	};
};
""")

    #
    # Test property deletion
    #

    verify_parse("""
/dts-v1/;

/ {
	keep = < 1 >;
	delete = < &sub >, &sub;
	/delete-property/ missing;
	/delete-property/ delete;
	sub: sub {
		y = < &sub >, &sub;
	};
};

&sub {
	/delete-property/ y;
};
""",
"""
/dts-v1/;

/ {
	keep = < 0x1 >;
	sub: sub {
	};
};
""")

    #
    # Test node deletion
    #

    verify_parse("""
/dts-v1/;

/ {
	sub1 {
		x = < 1 >;
		sub2 {
			x = < &sub >, &sub;
		};
		/delete-node/ sub2;
	};

	sub3: sub3 {
		x = < &sub >, &sub;
	};

	sub4 {
		x = < &sub >, &sub;
	};
};

/delete-node/ &sub3;
/delete-node/ &{/sub4};
""",
"""
/dts-v1/;

/ {
	sub1 {
		x = < 0x1 >;
	};
};
""")

    verify_error("""
/dts-v1/;

/ {
};

/delete-node/ &missing;
""",
".tmp.dts:6 (column 15): parse error: undefined node label 'missing'")

    verify_error("""
/dts-v1/;

/delete-node/ {
""",
".tmp.dts:3 (column 15): parse error: expected label (&foo) or path (&{/foo/bar}) reference")

    #
    # Test /include/ (which is handled in the lexer)
    #

    # Verify that /include/ searches the current directory

    open(".tmp/same-dir-1", "w").write("""
	x = [ 00 ];
	/include/ "same-dir-2"
""")
    open(".tmp/same-dir-2", "w").write("""
	y = [ 01 ];
	/include/ "same-dir-3"
""")
    open(".tmp/same-dir-3", "w").write("""
	z = [ 02 ];
""")

    verify_parse("""
/dts-v1/;

/ {
	/include/ ".tmp/same-dir-1"
};
""",
"""
/dts-v1/;

/ {
	x = [ 00 ];
	y = [ 01 ];
	z = [ 02 ];
};
""")

    # Test tricky includes and include paths

    open(".tmp2.dts", "w").write("""
/dts-v1/;
/ {
""")
    open(".tmp3.dts", "w").write("""
    x = <1>;
""")
    open(".tmp/via-include-path-1", "w").write("""
      = /include/ "via-include-path-2"
""")
    open(".tmp2/via-include-path-2", "w").write("""
        <2>;
};
""")

    verify_parse("""
/include/ ".tmp2.dts"
/include/ ".tmp3.dts"
y /include/ "via-include-path-1"
""",
"""
/dts-v1/;

/ {
	x = < 0x1 >;
	y = < 0x2 >;
};
""",
    include_path=(".tmp", ".tmp2",))

    verify_error("""
/include/ "missing"
""",
".tmp.dts:1 (column 1): parse error: 'missing' could not be found")

    # Verify that an error in an included file points to the right location

    open(".tmp2.dts", "w").write("""\


  x
""")

    verify_error("""


/include/ ".tmp2.dts"
""",
".tmp2.dts:3 (column 3): parse error: expected '/dts-v1/;' at start of file")

# Test recursive /include/ detection

    open(".tmp2.dts", "w").write("""\
/include/ ".tmp3.dts"
""")
    open(".tmp3.dts", "w").write("""\
/include/ ".tmp.dts"
""")

    verify_error("""
/include/ ".tmp2.dts"
""",
"""\
.tmp3.dts:1 (column 1): parse error: recursive /include/:
.tmp.dts:1 ->
.tmp2.dts:1 ->
.tmp3.dts:1 ->
.tmp.dts\
""")

    verify_error("""
/include/ ".tmp.dts"
""",
"""\
.tmp.dts:1 (column 1): parse error: recursive /include/:
.tmp.dts:1 ->
.tmp.dts\
""")

    #
    # Test /omit-if-no-ref/
    #

    verify_parse("""
/dts-v1/;

/ {
	x = < &{/referenced} >, &referenced2;

	/omit-if-no-ref/ referenced {
	};

	referenced2: referenced2 {
	};

	/omit-if-no-ref/ unreferenced {
	};

	l1: /omit-if-no-ref/ unreferenced2 {
	};

	/omit-if-no-ref/ l2: unreferenced3 {
	};

	unreferenced4: unreferenced4 {
	};

	unreferenced5 {
	};
};

/omit-if-no-ref/ &referenced2;
/omit-if-no-ref/ &unreferenced4;
/omit-if-no-ref/ &{/unreferenced5};
""",
"""
/dts-v1/;

/ {
	x = < &{/referenced} >, &referenced2;
	referenced {
		phandle = < 0x1 >;
	};
	referenced2: referenced2 {
	};
};
""")

    verify_error("""
/dts-v1/;

/ {
	/omit-if-no-ref/ x = "";
};
""",
".tmp.dts:4 (column 21): parse error: /omit-if-no-ref/ can only be used on nodes")

    verify_error("""
/dts-v1/;

/ {
	/omit-if-no-ref/ x;
};
""",
".tmp.dts:4 (column 20): parse error: /omit-if-no-ref/ can only be used on nodes")

    verify_error("""
/dts-v1/;

/ {
	/omit-if-no-ref/ {
	};
};
""",
".tmp.dts:4 (column 19): parse error: expected node or property name")

    verify_error("""
/dts-v1/;

/ {
	/omit-if-no-ref/ = < 0 >;
};
""",
".tmp.dts:4 (column 19): parse error: expected node or property name")

    verify_error("""
/dts-v1/;

/ {
};

/omit-if-no-ref/ &missing;
""",
".tmp.dts:6 (column 18): parse error: undefined node label 'missing'")

    verify_error("""
/dts-v1/;

/omit-if-no-ref/ {
""",
".tmp.dts:3 (column 18): parse error: expected label (&foo) or path (&{/foo/bar}) reference")

    #
    # Test expressions
    #

    verify_parse("""
/dts-v1/;

/ {
	ter1        = < (0 ? 1 : 0 ? 2 : 3) >;
	ter2        = < (0 ? 1 : 1 ? 2 : 3) >;
	ter3        = < (1 ? 1 : 0 ? 2 : 3) >;
	ter4        = < (1 ? 1 : 1 ? 2 : 3) >;
	or1         = < (0 || 0) >;
	or2         = < (0 || 1) >;
	or3         = < (1 || 0) >;
	or4         = < (1 || 1) >;
	and1        = < (0 && 0) >;
	and2        = < (0 && 1) >;
	and3        = < (1 && 0) >;
	and4        = < (1 && 1) >;
	bitor       = < (1 | 2) >;
	bitxor      = < (7 ^ 2) >;
	bitand      = < (3 & 6) >;
	eq1         = < (1 == 0) >;
	eq2         = < (1 == 1) >;
	neq1        = < (1 != 0) >;
	neq2        = < (1 != 1) >;
	lt1         = < (1 < 2) >;
	lt2         = < (2 < 2) >;
	lt3         = < (3 < 2) >;
	lteq1       = < (1 <= 2) >;
	lteq2       = < (2 <= 2) >;
	lteq3       = < (3 <= 2) >;
	gt1         = < (1 > 2) >;
	gt2         = < (2 > 2) >;
	gt3         = < (3 > 2) >;
	gteq1       = < (1 >= 2) >;
	gteq2       = < (2 >= 2) >;
	gteq3       = < (3 >= 2) >;
	lshift      = < (2 << 3) >;
	rshift      = < (16 >> 3) >;
	add         = < (3 + 4) >;
	sub         = < (7 - 4) >;
	mul         = < (3 * 4) >;
	div         = < (11 / 3) >;
	mod         = < (11 % 3) >;
	unary_minus = < (-3) >;
	bitnot      = < (~1) >;
	not0        = < (!-1) >;
	not1        = < (!0) >;
	not2        = < (!1) >;
	not3        = < (!2) >;
	nest        = < (((--3) + (-2)) * (--(-2))) >;
	char_lits   = < ('a' + 'b') >;
};
""",
"""
/dts-v1/;

/ {
	ter1 = < 0x3 >;
	ter2 = < 0x2 >;
	ter3 = < 0x1 >;
	ter4 = < 0x1 >;
	or1 = < 0x0 >;
	or2 = < 0x1 >;
	or3 = < 0x1 >;
	or4 = < 0x1 >;
	and1 = < 0x0 >;
	and2 = < 0x0 >;
	and3 = < 0x0 >;
	and4 = < 0x1 >;
	bitor = < 0x3 >;
	bitxor = < 0x5 >;
	bitand = < 0x2 >;
	eq1 = < 0x0 >;
	eq2 = < 0x1 >;
	neq1 = < 0x1 >;
	neq2 = < 0x0 >;
	lt1 = < 0x1 >;
	lt2 = < 0x0 >;
	lt3 = < 0x0 >;
	lteq1 = < 0x1 >;
	lteq2 = < 0x1 >;
	lteq3 = < 0x0 >;
	gt1 = < 0x0 >;
	gt2 = < 0x0 >;
	gt3 = < 0x1 >;
	gteq1 = < 0x0 >;
	gteq2 = < 0x1 >;
	gteq3 = < 0x1 >;
	lshift = < 0x10 >;
	rshift = < 0x2 >;
	add = < 0x7 >;
	sub = < 0x3 >;
	mul = < 0xc >;
	div = < 0x3 >;
	mod = < 0x2 >;
	unary_minus = < 0xfffffffd >;
	bitnot = < 0xfffffffe >;
	not0 = < 0x0 >;
	not1 = < 0x1 >;
	not2 = < 0x0 >;
	not3 = < 0x0 >;
	nest = < 0xfffffffe >;
	char_lits = < 0xc3 >;
};
""")

    verify_error("""
/dts-v1/;

/ {
	a = < (1/(-1 + 1)) >;
};
""",
".tmp.dts:4 (column 18): parse error: division by zero")

    verify_error("""
/dts-v1/;

/ {
	a = < (1%0) >;
};
""",
".tmp.dts:4 (column 11): parse error: division by zero")

    #
    # Test comment removal
    #

    verify_parse("""
/**//dts-v1//**/;//
//
// foo
/ /**/{// foo
x/**/=/*
foo
*/</**/1/***/>/****/;/**/}/*/**/;
""",
"""
/dts-v1/;

/ {
	x = < 0x1 >;
};
""")

    #
    # Test get_node()
    #

    def verify_path_is(path, node_name):
        try:
            node = dt.get_node(path)
            if node.name != node_name:
                fail("expected {} to lead to {}, lead to {}"
                     .format(path, node_name, node.name))
        except dtlib.DTError:
            fail("no node found for path " + path)

    def verify_path_error(path, msg):
        prefix = "expected looking up '{}' to generate the error '{}', " \
                 "generated ".format(path, msg)
        try:
            dt.get_node(path)
            fail(prefix + " no error")
        except dtlib.DTError as e:
            if str(e) != msg:
                fail("{} the error '{}'".format(prefix, e))
        except Exception as e:
            fail("{} the non-DTError '{}'".format(prefix, e))

    def verify_path_exists(path):
        if not dt.has_node(path):
            fail("expected path '{}' to exist, didn't".format(path))

    def verify_path_missing(path):
        if dt.has_node(path):
            fail("expected path '{}' to not exist, did".format(path))

    dt = parse("""
/dts-v1/;

/ {
	foo {
		bar {
		};
	};

	baz {
	};
};
""")

    verify_path_is("/", "/")
    verify_path_is("//", "/")
    verify_path_is("///", "/")
    verify_path_is("/foo", "foo")
    verify_path_is("//foo", "foo")
    verify_path_is("///foo", "foo")
    verify_path_is("/foo/bar", "bar")
    verify_path_is("//foo//bar", "bar")
    verify_path_is("///foo///bar", "bar")
    verify_path_is("/baz", "baz")

    verify_path_error("", "no alias '' found -- did you forget the leading '/' in the node path?")
    verify_path_error("missing", "no alias 'missing' found -- did you forget the leading '/' in the node path?")
    verify_path_error("/missing", "component 'missing' in path '/missing' does not exist")
    verify_path_error("/foo/missing", "component 'missing' in path '/foo/missing' does not exist")

    verify_path_exists("/")
    verify_path_exists("/foo")
    verify_path_exists("/foo/bar")

    verify_path_missing("/missing")
    verify_path_missing("/foo/missing")

    #
    # Test /aliases
    #

    def verify_alias_target(alias, node_name):
        verify_path_is(alias, node_name)
        if alias not in dt.alias2node:
            fail("expected {} to be in alias2node".format(alias))
        if dt.alias2node[alias].name != node_name:
            fail("expected alias {} to lead to {}, lead to {}"
                 .format(alias, node_name, dt.alias2node[alias].name))

    dt = parse("""
/dts-v1/;

/ {
	aliases {
		alias1 = &l1;
		alias2 = &l2;
		alias3 = &{/sub/node3};
		alias4 = &{/node4};
	};

	l1: node1 {
	};

	l2: node2 {
	};

	sub {
		node3 {
		};
	};

	node4 {
		node5 {
		};
	};
};
""")

    verify_alias_target("alias1", "node1")
    verify_alias_target("alias2", "node2")
    verify_alias_target("alias3", "node3")
    verify_path_is("alias4/node5", "node5")

    verify_path_error("alias4/node5/node6",
                      "component 'node6' in path 'alias4/node5/node6' does not exist")

    verify_error("""
/dts-v1/;

/ {
	aliases {
		a = [ 00 ];
	};
};
""",
"expected property 'a' on /aliases in .tmp.dts to be assigned with either 'a = &foo' or 'a = \"/path/to/node\"', not 'a = [ 00 ];'")

    verify_error(r"""
/dts-v1/;

/ {
	aliases {
		a = "\xFF";
	};
};
""",
r"value of property 'a' (b'\xff\x00') on /aliases in .tmp.dts is not valid UTF-8")

    verify_error("""
/dts-v1/;

/ {
	aliases {
		A = "/aliases";
	};
};
""",
"/aliases: alias property name 'A' should include only characters from [0-9a-z-]")

    verify_error(r"""
/dts-v1/;

/ {
	aliases {
		a = "/missing";
	};
};
""",
"property 'a' on /aliases in .tmp.dts points to the non-existent node \"/missing\"")

    #
    # Test Property.type
    #

    def verify_type(prop, expected):
        actual = dt.root.props[prop].type
        if actual != expected:
            fail("expected {} to have type {}, had type {}"
                 .format(prop, expected, actual))

    dt = parse("""
/dts-v1/;

/ {
	empty;
	bytes1 = [ ];
	bytes2 = [ 01 ];
	bytes3 = [ 01 02 ];
	bytes4 = foo: [ 01 bar: 02 ];
	bytes5 = /bits/ 8 < 1 2 3 >;
	num = < 1 >;
	nums1 = < >;
	nums2 = < >, < >;
	nums3 = < 1 2 >;
	nums4 = < 1 2 >, < 3 >, < 4 >;
	string = "foo";
	strings = "foo", "bar";
	path1 = &node;
	path2 = &{/node};
	phandle1 = < &node >;
	phandle2 = < &{/node} >;
	phandles1 = < &node &node >;
	phandles2 = < &node >, < &node >;
	phandle-and-nums-1 = < &node 1 >;
	phandle-and-nums-2 = < &node 1 2 &node 3 4 >;
	phandle-and-nums-3 = < &node 1 2 >, < &node 3 4 >;
	compound1 = < 1 >, [ 02 ];
	compound2 = "foo", < >;

	node: node {
	};
};
""")

    verify_type("empty", dtlib.TYPE_EMPTY)
    verify_type("bytes1", dtlib.TYPE_BYTES)
    verify_type("bytes2", dtlib.TYPE_BYTES)
    verify_type("bytes3", dtlib.TYPE_BYTES)
    verify_type("bytes4", dtlib.TYPE_BYTES)
    verify_type("bytes5", dtlib.TYPE_BYTES)
    verify_type("num", dtlib.TYPE_NUM)
    verify_type("nums1", dtlib.TYPE_NUMS)
    verify_type("nums2", dtlib.TYPE_NUMS)
    verify_type("nums3", dtlib.TYPE_NUMS)
    verify_type("nums4", dtlib.TYPE_NUMS)
    verify_type("string", dtlib.TYPE_STRING)
    verify_type("strings", dtlib.TYPE_STRINGS)
    verify_type("phandle1", dtlib.TYPE_PHANDLE)
    verify_type("phandle2", dtlib.TYPE_PHANDLE)
    verify_type("phandles1", dtlib.TYPE_PHANDLES)
    verify_type("phandles2", dtlib.TYPE_PHANDLES)
    verify_type("phandle-and-nums-1", dtlib.TYPE_PHANDLES_AND_NUMS)
    verify_type("phandle-and-nums-2", dtlib.TYPE_PHANDLES_AND_NUMS)
    verify_type("phandle-and-nums-3", dtlib.TYPE_PHANDLES_AND_NUMS)
    verify_type("path1", dtlib.TYPE_PATH)
    verify_type("path2", dtlib.TYPE_PATH)
    verify_type("compound1", dtlib.TYPE_COMPOUND)
    verify_type("compound2", dtlib.TYPE_COMPOUND)

    #
    # Test Property.to_{num,nums,string,strings,node}()
    #

    dt = parse(r"""
/dts-v1/;

/ {
	u = < 1 >;
	s = < 0xFFFFFFFF >;
	u8 = /bits/ 8 < 1 >;
	u16 = /bits/ 16 < 1 2 >;
	u64 = /bits/ 64 < 1 >;
        bytes = [ 01 02 03 ];
	empty;
	zero = < >;
	two_u = < 1 2 >;
	two_s = < 0xFFFFFFFF 0xFFFFFFFE >;
	three_u = < 1 2 3 >;
	three_u_split = < 1 >, < 2 >, < 3 >;
	empty_string = "";
	string = "foo\tbar baz";
	invalid_string = "\xff";
	strings = "foo", "bar", "baz";
	invalid_strings = "foo", "\xff", "bar";
	ref = <&{/target}>;
	refs = <&{/target} &{/target2}>;
	refs2 = <&{/target}>, <&{/target2}>;
	path = &{/target};
	manualpath = "/target";
	missingpath = "/missing";

	target {
		phandle = < 100 >;
	};

	target2 {
	};
};
""")

    # Test Property.to_num()

    def verify_to_num(prop, signed, expected):
        try:
            actual = dt.root.props[prop].to_num(signed)
        except dtlib.DTError as e:
            fail("failed to convert '{}' to {} number: {}"
                 .format(prop, "a signed" if signed else "an unsigned", e))

        if actual != expected:
            fail("expected {} to have the {} numeric value {:#x}, had the "
                 "value {:#x}".format(prop, "signed" if signed else "unsigned",
                                      expected, actual))

    def verify_to_num_error(prop, msg):
        prefix = "expected fetching '{}' as a number to generate the error " \
                 "'{}', generated".format(prop, msg)
        try:
            dt.root.props[prop].to_num()
            fail(prefix + " no error")
        except dtlib.DTError as e:
            if str(e) != msg:
                fail("{} the error '{}'".format(prefix, e))
        except Exception as e:
            fail("{} the non-DTError '{}'".format(prefix, e))

    verify_to_num("u", False, 1)
    verify_to_num("u", True, 1)
    verify_to_num("s", False, 0xFFFFFFFF)
    verify_to_num("s", True, -1)

    verify_to_num_error("two_u", "expected property 'two_u' on / in .tmp.dts to be assigned with 'two_u = < (number) >;', not 'two_u = < 0x1 0x2 >;'")
    verify_to_num_error("u8", "expected property 'u8' on / in .tmp.dts to be assigned with 'u8 = < (number) >;', not 'u8 = [ 01 ];'")
    verify_to_num_error("u16", "expected property 'u16' on / in .tmp.dts to be assigned with 'u16 = < (number) >;', not 'u16 = /bits/ 16 < 0x1 0x2 >;'")
    verify_to_num_error("u64", "expected property 'u64' on / in .tmp.dts to be assigned with 'u64 = < (number) >;', not 'u64 = /bits/ 64 < 0x1 >;'")
    verify_to_num_error("string", "expected property 'string' on / in .tmp.dts to be assigned with 'string = < (number) >;', not 'string = \"foo\\tbar baz\";'")

    # Test Property.to_nums()

    def verify_to_nums(prop, signed, expected):
        try:
            actual = dt.root.props[prop].to_nums(signed)
        except dtlib.DTError as e:
            fail("failed to convert '{}' to {} numbers: {}"
                 .format(prop, "signed" if signed else "unsigned", e))

        if actual != expected:
            fail("expected {} to give the {} numbers {}, gave {}"
                 .format(prop, "signed" if signed else "unsigned", expected,
                         actual))

    def verify_to_nums_error(prop, msg):
        prefix = "expected converting '{}' to numbers to generate the error " \
                 "'{}', generated".format(prop, msg)
        try:
            dt.root.props[prop].to_nums()
            fail(prefix + " no error")
        except dtlib.DTError as e:
            if str(e) != msg:
                fail("{} the error '{}'".format(prefix, e))
        except Exception as e:
            fail("{} the non-DTError '{}'".format(prefix, e))

    verify_to_nums("zero", False, [])
    verify_to_nums("u", False, [1])
    verify_to_nums("two_u", False, [1, 2])
    verify_to_nums("two_u", True, [1, 2])
    verify_to_nums("two_s", False, [0xFFFFFFFF, 0xFFFFFFFE])
    verify_to_nums("two_s", True, [-1, -2])
    verify_to_nums("three_u", False, [1, 2, 3])
    verify_to_nums("three_u_split", False, [1, 2, 3])

    verify_to_nums_error("empty", "expected property 'empty' on / in .tmp.dts to be assigned with 'empty = < (number) (number) ... >;', not 'empty;'")
    verify_to_nums_error("string", "expected property 'string' on / in .tmp.dts to be assigned with 'string = < (number) (number) ... >;', not 'string = \"foo\\tbar baz\";'")

    # Test Property.to_bytes()

    def verify_to_bytes(prop, expected):
        try:
            actual = dt.root.props[prop].to_bytes()
        except dtlib.DTError as e:
            fail("failed to convert '{}' to bytes: {}".format(prop, e))

        if actual != expected:
            fail("expected {} to give the bytes {}, gave {}"
                 .format(prop, expected, actual))

    def verify_to_bytes_error(prop, msg):
        prefix = "expected converting '{}' to bytes to generate the error " \
                 "'{}', generated".format(prop, msg)
        try:
            dt.root.props[prop].to_bytes()
            fail(prefix + " no error")
        except dtlib.DTError as e:
            if str(e) != msg:
                fail("{} the error '{}'".format(prefix, e))
        except Exception as e:
            fail("{} the non-DTError '{}'".format(prefix, e))

    verify_to_bytes("u8", b"\x01")
    verify_to_bytes("bytes", b"\x01\x02\x03")

    verify_to_bytes_error("u16", "expected property 'u16' on / in .tmp.dts to be assigned with 'u16 = [ (byte) (byte) ... ];', not 'u16 = /bits/ 16 < 0x1 0x2 >;'")
    verify_to_bytes_error("empty", "expected property 'empty' on / in .tmp.dts to be assigned with 'empty = [ (byte) (byte) ... ];', not 'empty;'")

    # Test Property.to_string()

    def verify_to_string(prop, expected):
        try:
            actual = dt.root.props[prop].to_string()
        except dtlib.DTError as e:
            fail("failed to convert '{}' to string: {}".format(prop, e))

        if actual != expected:
            fail("expected {} to have the value '{}', had the value '{}'"
                 .format(prop, expected, actual))

    def verify_to_string_error(prop, msg):
        prefix = "expected converting '{}' to string to generate the error " \
                 "'{}', generated".format(prop, msg)
        try:
            dt.root.props[prop].to_string()
            fail(prefix + " no error")
        except dtlib.DTError as e:
            if str(e) != msg:
                fail("{} the error '{}'".format(prefix, e))
        except Exception as e:
            fail("{} the non-DTError '{}'".format(prefix, e))

    verify_to_string("empty_string", "")
    verify_to_string("string", "foo\tbar baz")

    verify_to_string_error("u", "expected property 'u' on / in .tmp.dts to be assigned with 'u = \"string\";', not 'u = < 0x1 >;'")
    verify_to_string_error("strings", "expected property 'strings' on / in .tmp.dts to be assigned with 'strings = \"string\";', not 'strings = \"foo\", \"bar\", \"baz\";'")
    verify_to_string_error("invalid_string", r"value of property 'invalid_string' (b'\xff\x00') on / in .tmp.dts is not valid UTF-8")

    # Test Property.to_strings()

    def verify_to_strings(prop, expected):
        try:
            actual = dt.root.props[prop].to_strings()
        except dtlib.DTError as e:
            fail("failed to convert '{}' to strings: {}".format(prop, e))

        if actual != expected:
            fail("expected {} to have the value '{}', had the value '{}'"
                 .format(prop, expected, actual))

    def verify_to_strings_error(prop, msg):
        prefix = "expected converting '{}' to strings to generate the error " \
                 "'{}', generated".format(prop, msg)
        try:
            dt.root.props[prop].to_strings()
            fail(prefix + " no error")
        except dtlib.DTError as e:
            if str(e) != msg:
                fail("{} the error '{}'".format(prefix, e))
        except Exception as e:
            fail("{} the non-DTError '{}'".format(prefix, e))

    verify_to_strings("empty_string", [""])
    verify_to_strings("string", ["foo\tbar baz"])
    verify_to_strings("strings", ["foo", "bar", "baz"])

    verify_to_strings_error("u", "expected property 'u' on / in .tmp.dts to be assigned with 'u = \"string\", \"string\", ... ;', not 'u = < 0x1 >;'")
    verify_to_strings_error("invalid_strings", r"value of property 'invalid_strings' (b'foo\x00\xff\x00bar\x00') on / in .tmp.dts is not valid UTF-8")

    # Test Property.to_node()

    def verify_to_node(prop, path):
        try:
            actual = dt.root.props[prop].to_node().path
        except dtlib.DTError as e:
            fail("failed to convert '{}' to node: {}".format(prop, e))

        if actual != path:
            fail("expected {} to point to {}, pointed to {}"
                 .format(prop, path, actual))

    def verify_to_node_error(prop, msg):
        prefix = "expected converting '{}' to a node to generate the error " \
                 "'{}', generated".format(prop, msg)
        try:
            dt.root.props[prop].to_node()
            fail(prefix + " no error")
        except dtlib.DTError as e:
            if str(e) != msg:
                fail("{} the error '{}'".format(prefix, e))
        except Exception as e:
            fail("{} the non-DTError '{}'".format(prefix, e))

    verify_to_node("ref", "/target")

    verify_to_node_error("u", "expected property 'u' on / in .tmp.dts to be assigned with 'u = < &foo >;', not 'u = < 0x1 >;'")
    verify_to_node_error("string", "expected property 'string' on / in .tmp.dts to be assigned with 'string = < &foo >;', not 'string = \"foo\\tbar baz\";'")

    # Test Property.to_nodes()

    def verify_to_nodes(prop, paths):
        try:
            actual = [node.path for node in dt.root.props[prop].to_nodes()]
        except dtlib.DTError as e:
            fail("failed to convert '{}' to nodes: {}".format(prop, e))

        if actual != paths:
            fail("expected {} to point to the paths {}, pointed to {}"
                 .format(prop, paths, actual))

    def verify_to_nodes_error(prop, msg):
        prefix = "expected converting '{}' to a nodes to generate the error " \
                 "'{}', generated".format(prop, msg)
        try:
            dt.root.props[prop].to_nodes()
            fail(prefix + " no error")
        except dtlib.DTError as e:
            if str(e) != msg:
                fail("{} the error '{}'".format(prefix, e))
        except Exception as e:
            fail("{} the non-DTError '{}'".format(prefix, e))

    verify_to_nodes("zero", [])
    verify_to_nodes("ref", ["/target"])
    verify_to_nodes("refs", ["/target", "/target2"])
    verify_to_nodes("refs2", ["/target", "/target2"])

    verify_to_nodes_error("u", "expected property 'u' on / in .tmp.dts to be assigned with 'u = < &foo &bar ... >;', not 'u = < 0x1 >;'")
    verify_to_nodes_error("string", "expected property 'string' on / in .tmp.dts to be assigned with 'string = < &foo &bar ... >;', not 'string = \"foo\\tbar baz\";'")

    # Test Property.to_path()

    def verify_to_path(prop, path):
        try:
            actual = dt.root.props[prop].to_path().path
        except dtlib.DTError as e:
            fail("failed to convert '{}' to path: {}".format(prop, e))

        if actual != path:
            fail("expected {} to contain the path {}, contained {}"
                 .format(prop, path, actual))

    def verify_to_path_error(prop, msg):
        prefix = "expected converting '{}' to a path to generate the error " \
                 "'{}', generated".format(prop, msg)
        try:
            dt.root.props[prop].to_path()
            fail(prefix + " no error")
        except dtlib.DTError as e:
            if str(e) != msg:
                fail("{} the error '{}'".format(prefix, e))
        except Exception as e:
            fail("{} the non-DTError '{}'".format(prefix, e))

    verify_to_path("path", "/target")
    verify_to_path("manualpath", "/target")

    verify_to_path_error("u", "expected property 'u' on / in .tmp.dts to be assigned with either 'u = &foo' or 'u = \"/path/to/node\"', not 'u = < 0x1 >;'")
    verify_to_path_error("missingpath", "property 'missingpath' on / in .tmp.dts points to the non-existent node \"/missing\"")

    # Test top-level to_num() and to_nums()

    def verify_raw_to_num(fn, prop, length, signed, expected):
        try:
            actual = fn(dt.root.props[prop].value, length, signed)
        except dtlib.DTError as e:
            fail("failed to convert '{}' to {} number(s) with {}: {}"
                 .format(prop, "signed" if signed else "unsigned",
                         fn.__name__, e))

        if actual != expected:
            fail("expected {}(<{}>, {}, {}) to be {}, was {}"
                 .format(fn.__name__, prop, length, signed, expected, actual))

    def verify_raw_to_num_error(fn, data, length, msg):
        prefix = "expected {}() called with data='{}', length='{}' to " \
                 "generate the error '{}', generated" \
                 .format(fn.__name__, data, length, msg)
        try:
            fn(data, length)
            fail(prefix + " no error")
        except dtlib.DTError as e:
            if str(e) != msg:
                fail("{} the error '{}'".format(prefix, e))
        except Exception as e:
            fail("{} the non-DTError '{}'".format(prefix, e))

    verify_raw_to_num(dtlib.to_num, "u", None, False, 1)
    verify_raw_to_num(dtlib.to_num, "u", 4, False, 1)
    verify_raw_to_num(dtlib.to_num, "s", None, False, 0xFFFFFFFF)
    verify_raw_to_num(dtlib.to_num, "s", None, True, -1)
    verify_raw_to_num(dtlib.to_nums, "empty", 4, False, [])
    verify_raw_to_num(dtlib.to_nums, "u16", 2, False, [1, 2])
    verify_raw_to_num(dtlib.to_nums, "two_s", 4, False, [0xFFFFFFFF, 0xFFFFFFFE])
    verify_raw_to_num(dtlib.to_nums, "two_s", 4, True, [-1, -2])

    verify_raw_to_num_error(dtlib.to_num, 0, 0, "'0' has type 'int', expected 'bytes'")
    verify_raw_to_num_error(dtlib.to_num, b"", 0, "'length' must be greater than zero, was 0")
    verify_raw_to_num_error(dtlib.to_num, b"foo", 2, "b'foo' is 3 bytes long, expected 2")
    verify_raw_to_num_error(dtlib.to_nums, 0, 0, "'0' has type 'int', expected 'bytes'")
    verify_raw_to_num_error(dtlib.to_nums, b"", 0, "'length' must be greater than zero, was 0")
    verify_raw_to_num_error(dtlib.to_nums, b"foooo", 2, "b'foooo' is 5 bytes long, expected a length that's a a multiple of 2")

    #
    # Test duplicate label error
    #

    verify_error("""
/dts-v1/;

/ {
	sub1 {
		label: foo {
		};
	};

	sub2 {
		label: bar {
		};
	};
};
""",
"Label 'label' appears on /sub1/foo and on /sub2/bar")

    verify_error("""
/dts-v1/;

/ {
	sub {
		label: foo {
		};
	};
};
/ {
	sub {
		label: bar {
		};
	};
};
""",
"Label 'label' appears on /sub/bar and on /sub/foo")

    verify_error("""
/dts-v1/;

/ {
	foo: a = < 0 >;
	foo: node {
	};
};
""",
"Label 'foo' appears on /node and on property 'a' of node /")

    verify_error("""
/dts-v1/;

/ {
	foo: a = < 0 >;
	node {
		foo: b = < 0 >;
	};
};
""",
"Label 'foo' appears on property 'a' of node / and on property 'b' of node /node")

    verify_error("""
/dts-v1/;

/ {
	foo: a = foo: < 0 >;
};
""",
"Label 'foo' appears in the value of property 'a' of node / and on property 'a' of node /")

    # Giving the same label twice for the same node is fine
    verify_parse("""
/dts-v1/;

/ {
	sub {
		label: foo {
		};
	};
};
/ {

	sub {
		label: foo {
		};
	};
};
""",
"""
/dts-v1/;

/ {
	sub {
		label: foo {
		};
	};
};
""")

    # Duplicate labels are fine if one of the nodes is deleted
    verify_parse("""
/dts-v1/;

/ {
	label: foo {
	};
	label: bar {
	};
};

/delete-node/ &{/bar};
""",
"""
/dts-v1/;

/ {
	label: foo {
	};
};
""")

    #
    # Test overriding/deleting a property with references
    #

    verify_parse("""
/dts-v1/;

/ {
	x = &foo, < &foo >;
	y = &foo, < &foo >;
	foo: foo {
	};
};

/ {
	x = < 1 >;
	/delete-property/ y;
};
""",
"""
/dts-v1/;

/ {
	x = < 0x1 >;
	foo: foo {
	};
};
""")

    #
    # Test self-referential node
    #

    verify_parse("""
/dts-v1/;

/ {
	label: foo {
		x = &{/foo}, &label, < &label >;
	};
};
""",
"""
/dts-v1/;

/ {
	label: foo {
		x = &{/foo}, &label, < &label >;
		phandle = < 0x1 >;
	};
};
""")

    #
    # Test /memreserve/
    #

    dt = verify_parse("""
/dts-v1/;

l1: l2: /memreserve/ (1 + 1) (2 * 2);
/memreserve/ 0x100 0x200;

/ {
};
""",
"""
/dts-v1/;

l1: l2: /memreserve/ 0x0000000000000002 0x0000000000000004;
/memreserve/ 0x0000000000000100 0x0000000000000200;

/ {
};
""")

    expected = [(["l1", "l2"], 2, 4), ([], 0x100, 0x200)]
    if dt.memreserves != expected:
        fail("expected {} for dt.memreserve, got {}"
             .format(expected, dt.memreserves))

    verify_error("""
/dts-v1/;

foo: / {
};
""",
".tmp.dts:3 (column 6): parse error: expected /memreserve/ after labels at beginning of file")

    #
    # Test __repr__() functions
    #

    def verify_repr(obj, expected):
        if repr(obj) != expected:
            fail("expected repr() to be '{}', was '{}'"
                 .format(expected, repr(obj)))

    dt = parse("""
/dts-v1/;

/ {
	x = < 0 >;
	sub {
		y = < 1 >;
	};
};
""",
    include_path=("foo", "bar"))

    verify_repr(dt, "DT(filename='.tmp.dts', include_path=('foo', 'bar'))")
    verify_repr(dt.root.props["x"], "<Property 'x' at '/' in '.tmp.dts'>")
    verify_repr(dt.root.nodes["sub"], "<Node /sub in '.tmp.dts'>")

    #
    # Test names
    #

    # The C tools disallow '@' in property names, but otherwise accept the same
    # characters in node and property names. Emulate that instead of the DT spec
    # (v0.2), which gives different characters for nodes and properties.
    verify_parse(r"""
/dts-v1/;

/ {
	// A leading \ is accepted but ignored in node/propert names
	\aA0,._+*#?- = &_, &{/aA0,._+*#?@-};

	// Names that overlap with operators and integer literals

	+ = [ 00 ];
	* = [ 02 ];
	- = [ 01 ];
	? = [ 03 ];
	0 = [ 04 ];
	0x123 = [ 05 ];

	_: \aA0,._+*#?@- {
	};

	0 {
	};
};
""",
"""
/dts-v1/;

/ {
	aA0,._+*#?- = &_, &{/aA0,._+*#?@-};
	+ = [ 00 ];
	* = [ 02 ];
	- = [ 01 ];
	? = [ 03 ];
	0 = [ 04 ];
	0x123 = [ 05 ];
	_: aA0,._+*#?@- {
	};
	0 {
	};
};
""")

    verify_error(r"""
/dts-v1/;

/ {
	foo@3;
};
""",
".tmp.dts:4 (column 7): parse error: '@' is only allowed in node names")

    verify_error(r"""
/dts-v1/;

/ {
	foo@3 = < 0 >;
};
""",
".tmp.dts:4 (column 8): parse error: '@' is only allowed in node names")

    verify_error(r"""
/dts-v1/;

/ {
	foo@2@3 {
	};
};
""",
".tmp.dts:4 (column 10): parse error: multiple '@' in node name")

    #
    # Test bad formatting
    #

    verify_parse("""
/dts-v1/;/{l1:l2:foo{l3:l4:bar{l5:x=l6:/bits/8<l7:1 l8:2>l9:,[03],"a";};};};
""",
"""
/dts-v1/;

/ {
	l1: l2: foo {
		l3: l4: bar {
			l5: x = l6: [ l7: 01 l8: 02 l9: ], [ 03 ], "a";
		};
	};
};
""")

    #
    # Test misc. errors and non-errors
    #

    verify_error("", ".tmp.dts:1 (column 1): parse error: expected '/dts-v1/;' at start of file")

    verify_error("""
/dts-v1/;
""",
".tmp.dts:2 (column 1): parse error: no root node defined")

    verify_error("""
/dts-v1/; /plugin/;
""",
".tmp.dts:1 (column 11): parse error: /plugin/ is not supported")

    verify_error("""
/dts-v1/;

/ {
	foo: foo {
	};
};

// Only one label supported before label references at the top level
l1: l2: &foo {
};
""",
".tmp.dts:9 (column 5): parse error: expected label reference (&foo)")

    verify_error("""
/dts-v1/;

/ {
        foo: {};
};
""",
".tmp.dts:4 (column 14): parse error: expected node or property name")

    # Multiple /dts-v1/ at the start of a file is fine
    verify_parse("""
/dts-v1/;
/dts-v1/;

/ {
};
""",
"""
/dts-v1/;

/ {
};
""")

    print("all tests passed")

    # Only remove these if tests succeed. They're handy to have around otherwise.
    shutil.rmtree(".tmp")
    shutil.rmtree(".tmp2")
    os.remove(".tmp.dts")
    os.remove(".tmp2.dts")
    os.remove(".tmp3.dts")
    os.remove(".tmp.bin")


if __name__ == "__main__":
    run()
