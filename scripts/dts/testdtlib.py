# Copyright (c) 2019, Nordic Semiconductor
# SPDX-License-Identifier: BSD-3-Clause

import contextlib
import os
import re
import tempfile

import pytest

import dtlib

# Test suite for dtlib.py.
#
# Run it using pytest (https://docs.pytest.org/en/stable/usage.html):
#
#   $ pytest testdtlib.py
#
# Extra options you can pass to pytest for debugging:
#
#   - to stop on the first failure with shorter traceback output,
#     use '-x --tb=native'
#   - to drop into a debugger on failure, use '--pdb'
#   - to run a particular test function or functions, use
#     '-k test_function_pattern_goes_here'

def parse(dts, include_path=()):
    '''Parse a DTS string 'dts', using the given include path.'''

    fd, path = tempfile.mkstemp(prefix='pytest-', suffix='.dts')
    try:
        os.write(fd, dts.encode('utf-8'))
        return dtlib.DT(path, include_path)
    finally:
        os.close(fd)
        os.unlink(path)

def verify_parse(dts, expected, include_path=()):
    '''Like parse(), but also verifies that the parsed DT object's string
    representation is expected[1:-1].

    The [1:] is so that the first line can be put on a separate line
    after triple quotes, as is done below.'''

    dt = parse(dts[1:], include_path)

    actual = str(dt)
    expected = expected[1:-1]
    assert actual == expected, f'unexpected round-trip on {dts}'

    return dt

def verify_error(dts, expected_msg):
    '''Verify that parsing 'dts' results in a DTError with the
    given error message 'msg'. The message must match exactly.'''

    with pytest.raises(dtlib.DTError) as e:
        parse(dts[1:])
    actual_msg = str(e.value)
    assert actual_msg == expected_msg, f'wrong error from {dts}'

def verify_error_endswith(dts, expected_msg):
    '''
    Like verify_error(), but checks the message ends with
    'expected_msg' instead of checking for strict equality.
    '''

    with pytest.raises(dtlib.DTError) as e:
        parse(dts[1:])
    actual_msg = str(e.value)
    assert actual_msg.endswith(expected_msg), f'wrong error from {dts}'

def verify_error_matches(dts, expected_re):
    '''
    Like verify_error(), but checks the message fully matches regular
    expression 'expected_re' instead of checking for strict equality.
    '''

    with pytest.raises(dtlib.DTError) as e:
        parse(dts[1:])
    actual_msg = str(e.value)
    assert re.fullmatch(expected_re, actual_msg), \
        f'wrong error from {dts}' \
        f'actual message:\n{actual_msg!r}\n' \
        f'does not match:\n{expected_re!r}'

@contextlib.contextmanager
def temporary_chdir(dirname):
    '''A context manager that changes directory to 'dirname'.

    The current working directory is unconditionally returned to its
    present location after the context manager exits.
    '''
    here = os.getcwd()
    try:
        os.chdir(dirname)
        yield
    finally:
        os.chdir(here)

def test_cell_parsing():
    '''Miscellaneous properties containing zero or more cells'''

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

    verify_error_endswith("""
/dts-v1/;

/ {
	a = /bits/ 16 < 0x10000 >;
};
""",
":4 (column 18): parse error: 65536 does not fit in 16 bits")

    verify_error_endswith("""
/dts-v1/;

/ {
	a = < 0x100000000 >;
};
""",
":4 (column 8): parse error: 4294967296 does not fit in 32 bits")

    verify_error_endswith("""
/dts-v1/;

/ {
	a = /bits/ 128 < 0 >;
};
""",
":4 (column 13): parse error: expected 8, 16, 32, or 64")

def test_bytes_parsing():
    '''Properties with byte array values'''

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

    verify_error_endswith("""
/dts-v1/;

/ {
	a = [ 123 ];
};
""",
":4 (column 10): parse error: expected two-digit byte or ']'")

def test_string_parsing():
    '''Properties with string values'''

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

    verify_error_endswith(r"""
/dts-v1/;

/ {
	a = "\400";
};
""",
":4 (column 6): parse error: octal escape out of range (> 255)")

def test_char_literal_parsing():
    '''Properties with character literal values'''

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

    verify_error_endswith("""
/dts-v1/;

/ {
	// Character literals are not allowed at the top level
	a = 'x';
};
""",
":5 (column 6): parse error: malformed value")

    verify_error_endswith("""
/dts-v1/;

/ {
	a = < '' >;
};
""",
":4 (column 7): parse error: character literals must be length 1")

    verify_error_endswith("""
/dts-v1/;

/ {
	a = < '12' >;
};
""",
":4 (column 7): parse error: character literals must be length 1")

def test_incbin(tmp_path):
    '''Test /incbin/, an undocumented feature that allows for
    binary file inclusion.

    https://github.com/dgibson/dtc/commit/e37ec7d5889fa04047daaa7a4ff55150ed7954d4'''

    open(tmp_path / "tmp_bin", "wb").write(b"\00\01\02\03")

    verify_parse(f"""
/dts-v1/;

/ {{
	a = /incbin/ ("{tmp_path}/tmp_bin");
	b = /incbin/ ("{tmp_path}/tmp_bin", 1, 1);
	c = /incbin/ ("{tmp_path}/tmp_bin", 1, 2);
}};
""",
"""
/dts-v1/;

/ {
	a = [ 00 01 02 03 ];
	b = [ 01 ];
	c = [ 01 02 ];
};
""")

    verify_parse("""
/dts-v1/;

/ {
	a = /incbin/ ("tmp_bin");
};
""",
"""
/dts-v1/;

/ {
	a = [ 00 01 02 03 ];
};
""",
                 include_path=(tmp_path,))

    verify_error_endswith(r"""
/dts-v1/;

/ {
	a = /incbin/ ("missing");
};
""",
":4 (column 25): parse error: 'missing' could not be found")

def test_node_merging():
    '''
    Labels and properties specified for the same node in different
    statements should be merged.
    '''

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

    verify_error_endswith("""
/dts-v1/;

/ {
};

&missing {
};
""",
":6 (column 1): parse error: undefined node label 'missing'")

    verify_error_endswith("""
/dts-v1/;

/ {
};

&{foo} {
};
""",
":6 (column 1): parse error: node path 'foo' does not start with '/'")

    verify_error_endswith("""
/dts-v1/;

/ {
};

&{/foo} {
};
""",
":6 (column 1): parse error: component 'foo' in path '/foo' does not exist")

def test_property_labels():
    '''Like nodes, properties can have labels too.'''

    def verify_label2prop(label, expected):
        actual = dt.label2prop[label].name
        assert actual == expected, f"label '{label}' mapped to wrong property"

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

def test_property_offset_labels():
    '''
    It's possible to give labels to data at nonnegative byte offsets
    within a property value.
    '''

    def verify_label2offset(label, expected_prop, expected_offset):
        actual_prop, actual_offset = dt.label2prop_offset[label]
        actual_prop = actual_prop.name
        assert (actual_prop, actual_offset) == \
            (expected_prop, expected_offset), \
            f"label '{label}' maps to wrong offset or property"

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

def test_unit_addr():
    '''Node unit addresses must be correctly extracted from their names.'''

    def verify_unit_addr(path, expected):
        node = dt.get_node(path)
        assert node.unit_addr == expected, \
            f"{node!r} has unexpected unit address"

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

def test_node_path_references():
    '''Node phandles may be specified using a reference to the node's path.'''

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

def test_phandles():
    '''Various tests related to phandles.'''

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

    verify_error_endswith("""
/dts-v1/;

/ {
	a: sub {
		x = /bits/ 16 < &a >;
	};
};
""",
":5 (column 19): parse error: phandle references are only allowed in arrays with 32-bit elements")

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

def test_phandle2node():
    '''Test the phandle2node dict in a dt instance.'''

    def verify_phandle2node(prop, offset, expected_name):
        phandle = dtlib.to_num(dt.root.props[prop].value[offset:offset + 4])
        actual_name = dt.phandle2node[phandle].name

        assert actual_name == expected_name, \
            f"'{prop}' is a phandle for the wrong thing"

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

def test_mixed_assign():
    '''Test mixed value type assignments'''

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

def test_deletion():
    '''Properties and nodes may be deleted from the tree.'''

    # Test property deletion

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

    # Test node deletion

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

    verify_error_endswith("""
/dts-v1/;

/ {
};

/delete-node/ &missing;
""",
":6 (column 15): parse error: undefined node label 'missing'")

    verify_error_endswith("""
/dts-v1/;

/delete-node/ {
""",
":3 (column 15): parse error: expected label (&foo) or path (&{/foo/bar}) reference")

def test_include_curdir(tmp_path):
    '''Verify that /include/ (which is handled in the lexer) searches the
    current directory'''

    with temporary_chdir(tmp_path):
        with open("same-dir-1", "w") as f:
            f.write("""
    x = [ 00 ];
    /include/ "same-dir-2"
""")
        with open("same-dir-2", "w") as f:
            f.write("""
    y = [ 01 ];
    /include/ "same-dir-3"
""")
        with open("same-dir-3", "w") as f:
            f.write("""
    z = [ 02 ];
""")
        with open("test.dts", "w") as f:
            f.write("""
/dts-v1/;

/ {
	/include/ "same-dir-1"
};
""")
        dt = dtlib.DT("test.dts")
        assert str(dt) == """
/dts-v1/;

/ {
	x = [ 00 ];
	y = [ 01 ];
	z = [ 02 ];
};
"""[1:-1]

def test_include_is_lexical(tmp_path):
    '''/include/ is done in the lexer, which means that property
    definitions can span multiple included files in different
    directories.'''

    with open(tmp_path / "tmp2.dts", "w") as f:
        f.write("""
/dts-v1/;
/ {
""")
    with open(tmp_path / "tmp3.dts", "w") as f:
        f.write("""
    x = <1>;
""")

    subdir_1 = tmp_path / "subdir-1"
    subdir_1.mkdir()
    with open(subdir_1 / "via-include-path-1", "w") as f:
        f.write("""
      = /include/ "via-include-path-2"
""")

    subdir_2 = tmp_path / "subdir-2"
    subdir_2.mkdir()
    with open(subdir_2 / "via-include-path-2", "w") as f:
        f.write("""
        <2>;
};
""")

    with open(tmp_path / "test.dts", "w") as test_dts:
        test_dts.write("""
/include/ "tmp2.dts"
/include/ "tmp3.dts"
y /include/ "via-include-path-1"
""")

    with temporary_chdir(tmp_path):
        dt = dtlib.DT("test.dts", include_path=(subdir_1, subdir_2))
    expected_dt = """
/dts-v1/;

/ {
	x = < 0x1 >;
	y = < 0x2 >;
};
"""[1:-1]
    assert str(dt) == expected_dt

def test_include_misc(tmp_path):
    '''Miscellaneous /include/ tests.'''

    # Missing includes should error out.

    verify_error_endswith("""
/include/ "missing"
""",
":1 (column 1): parse error: 'missing' could not be found")

    # Verify that an error in an included file points to the right location

    with temporary_chdir(tmp_path):
        with open("tmp2.dts", "w") as f:
            f.write("""\


  x
""")
        with open("tmp.dts", "w") as f:
            f.write("""


/include/ "tmp2.dts"
""")
        with pytest.raises(dtlib.DTError) as e:
            dtlib.DT("tmp.dts")

        assert str(e.value) == \
            "tmp2.dts:3 (column 3): parse error: expected '/dts-v1/;' at start of file"

def test_include_recursion(tmp_path):
    '''Test recursive /include/ detection'''

    with temporary_chdir(tmp_path):
        with open("tmp2.dts", "w") as f:
            f.write('/include/ "tmp3.dts"\n')
        with open("tmp3.dts", "w") as f:
            f.write('/include/ "tmp.dts"\n')

        with open("tmp.dts", "w") as f:
            f.write('/include/ "tmp2.dts"\n')
        with pytest.raises(dtlib.DTError) as e:
            dtlib.DT("tmp.dts")

        expected_err = """\
tmp3.dts:1 (column 1): parse error: recursive /include/:
tmp.dts:1 ->
tmp2.dts:1 ->
tmp3.dts:1 ->
tmp.dts"""
        assert str(e.value) == expected_err

        with open("tmp.dts", "w") as f:
            f.write('/include/ "tmp.dts"\n')
        with pytest.raises(dtlib.DTError) as e:
            dtlib.DT("tmp.dts")
        expected_err = """\
tmp.dts:1 (column 1): parse error: recursive /include/:
tmp.dts:1 ->
tmp.dts"""
        assert str(e.value) == expected_err

def test_omit_if_no_ref():
    '''The /omit-if-no-ref/ marker is a bit of undocumented
    dtc magic that removes a node from the tree if it isn't
    referred to elsewhere.

    https://elinux.org/Device_Tree_Source_Undocumented
    '''

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

    verify_error_endswith("""
/dts-v1/;

/ {
	/omit-if-no-ref/ x = "";
};
""",
":4 (column 21): parse error: /omit-if-no-ref/ can only be used on nodes")

    verify_error_endswith("""
/dts-v1/;

/ {
	/omit-if-no-ref/ x;
};
""",
":4 (column 20): parse error: /omit-if-no-ref/ can only be used on nodes")

    verify_error_endswith("""
/dts-v1/;

/ {
	/omit-if-no-ref/ {
	};
};
""",
":4 (column 19): parse error: expected node or property name")

    verify_error_endswith("""
/dts-v1/;

/ {
	/omit-if-no-ref/ = < 0 >;
};
""",
":4 (column 19): parse error: expected node or property name")

    verify_error_endswith("""
/dts-v1/;

/ {
};

/omit-if-no-ref/ &missing;
""",
":6 (column 18): parse error: undefined node label 'missing'")

    verify_error_endswith("""
/dts-v1/;

/omit-if-no-ref/ {
""",
":3 (column 18): parse error: expected label (&foo) or path (&{/foo/bar}) reference")

def test_expr():
    '''Property values may contain expressions.'''

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

    verify_error_endswith("""
/dts-v1/;

/ {
	a = < (1/(-1 + 1)) >;
};
""",
":4 (column 18): parse error: division by zero")

    verify_error_endswith("""
/dts-v1/;

/ {
	a = < (1%0) >;
};
""",
":4 (column 11): parse error: division by zero")

def test_comment_removal():
    '''Comments should be removed when round-tripped to a str.'''

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

def verify_path_is(path, node_name, dt):
    '''Verify 'node.name' matches 'node_name' in 'dt'.'''

    try:
        node = dt.get_node(path)
        assert node.name == node_name, f'unexpected path {path}'
    except dtlib.DTError:
        assert False, f'no node found for path {path}'

def verify_path_error(path, msg, dt):
    '''Verify that an attempt to get node 'path' from 'dt' raises
    a DTError whose str is 'msg'.'''

    with pytest.raises(dtlib.DTError) as e:
        dt.get_node(path)
    assert str(e.value) == msg, f"'{path}' gives the wrong error"

def test_get_node():
    '''Test DT.get_node().'''

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

    verify_path_is("/", "/", dt)
    verify_path_is("//", "/", dt)
    verify_path_is("///", "/", dt)
    verify_path_is("/foo", "foo", dt)
    verify_path_is("//foo", "foo", dt)
    verify_path_is("///foo", "foo", dt)
    verify_path_is("/foo/bar", "bar", dt)
    verify_path_is("//foo//bar", "bar", dt)
    verify_path_is("///foo///bar", "bar", dt)
    verify_path_is("/baz", "baz", dt)

    verify_path_error(
        "",
        "no alias '' found -- did you forget the leading '/' in the node path?",
        dt)
    verify_path_error(
        "missing",
        "no alias 'missing' found -- did you forget the leading '/' in the node path?",
        dt)
    verify_path_error(
        "/missing",
        "component 'missing' in path '/missing' does not exist",
        dt)
    verify_path_error(
        "/foo/missing",
        "component 'missing' in path '/foo/missing' does not exist",
        dt)

    def verify_path_exists(path):
        assert dt.has_node(path), f"path '{path}' does not exist"

    def verify_path_missing(path):
        assert not dt.has_node(path), f"path '{path}' exists"

    verify_path_exists("/")
    verify_path_exists("/foo")
    verify_path_exists("/foo/bar")

    verify_path_missing("/missing")
    verify_path_missing("/foo/missing")

def test_aliases():
    '''Test /aliases'''

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

    def verify_alias_target(alias, node_name):
        verify_path_is(alias, node_name, dt)
        assert alias in dt.alias2node
        assert dt.alias2node[alias].name == node_name, f"bad result for {alias}"

    verify_alias_target("alias1", "node1")
    verify_alias_target("alias2", "node2")
    verify_alias_target("alias3", "node3")
    verify_path_is("alias4/node5", "node5", dt)

    verify_path_error(
        "alias4/node5/node6",
        "component 'node6' in path 'alias4/node5/node6' does not exist",
        dt)

    verify_error_matches("""
/dts-v1/;

/ {
	aliases {
		a = [ 00 ];
	};
};
""",
"expected property 'a' on /aliases in .*" +
re.escape("to be assigned with either 'a = &foo' or 'a = \"/path/to/node\"', not 'a = [ 00 ];'"))

    verify_error_matches(r"""
/dts-v1/;

/ {
	aliases {
		a = "\xFF";
	};
};
""",
re.escape(r"value of property 'a' (b'\xff\x00') on /aliases in ") +
".* is not valid UTF-8")

    verify_error("""
/dts-v1/;

/ {
	aliases {
		A = "/aliases";
	};
};
""",
"/aliases: alias property name 'A' should include only characters from [0-9a-z-]")

    verify_error_matches(r"""
/dts-v1/;

/ {
	aliases {
		a = "/missing";
	};
};
""",
"property 'a' on /aliases in .* points to the non-existent node \"/missing\"")

def test_prop_type():
    '''Test Property.type'''

    def verify_type(prop, expected):
        actual = dt.root.props[prop].type
        assert actual == expected, f'{prop} has wrong type'

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

def test_prop_type_casting():
    '''Test Property.to_{num,nums,string,strings,node}()'''

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
        signed_str = "a signed" if signed else "an unsigned"
        actual = dt.root.props[prop].to_num(signed)
        assert actual == expected, \
            f"{prop} has bad {signed_str} numeric value"

    def verify_to_num_error_matches(prop, expected_re):
        with pytest.raises(dtlib.DTError) as e:
            dt.root.props[prop].to_num()
        actual_msg = str(e.value)
        assert re.fullmatch(expected_re, actual_msg), \
            f"'{prop}' to_num gives the wrong error: " \
            f"actual message:\n{actual_msg!r}\n" \
            f"does not match:\n{expected_re!r}"

    verify_to_num("u", False, 1)
    verify_to_num("u", True, 1)
    verify_to_num("s", False, 0xFFFFFFFF)
    verify_to_num("s", True, -1)

    verify_to_num_error_matches(
        "two_u",
        "expected property 'two_u' on / in .* to be assigned with " +
        re.escape("'two_u = < (number) >;', not 'two_u = < 0x1 0x2 >;'"))
    verify_to_num_error_matches(
        "u8",
        "expected property 'u8' on / in .* to be assigned with " +
        re.escape("'u8 = < (number) >;', not 'u8 = [ 01 ];'"))
    verify_to_num_error_matches(
        "u16",
        "expected property 'u16' on / in .* to be assigned with " +
        re.escape("'u16 = < (number) >;', not 'u16 = /bits/ 16 < 0x1 0x2 >;'"))
    verify_to_num_error_matches(
        "u64",
        "expected property 'u64' on / in .* to be assigned with " +
        re.escape("'u64 = < (number) >;', not 'u64 = /bits/ 64 < 0x1 >;'"))
    verify_to_num_error_matches(
        "string",
        "expected property 'string' on / in .* to be assigned with " +
        re.escape("'string = < (number) >;', not 'string = \"foo\\tbar baz\";'"))

    # Test Property.to_nums()

    def verify_to_nums(prop, signed, expected):
        signed_str = "signed" if signed else "unsigned"
        actual = dt.root.props[prop].to_nums(signed)
        assert actual == expected, \
            f"'{prop}' gives the wrong {signed_str} numbers"

    def verify_to_nums_error_matches(prop, expected_re):
        with pytest.raises(dtlib.DTError) as e:
            dt.root.props[prop].to_nums()
        assert re.fullmatch(expected_re, str(e.value)), \
            f"'{prop}' to_nums gives the wrong error"

    verify_to_nums("zero", False, [])
    verify_to_nums("u", False, [1])
    verify_to_nums("two_u", False, [1, 2])
    verify_to_nums("two_u", True, [1, 2])
    verify_to_nums("two_s", False, [0xFFFFFFFF, 0xFFFFFFFE])
    verify_to_nums("two_s", True, [-1, -2])
    verify_to_nums("three_u", False, [1, 2, 3])
    verify_to_nums("three_u_split", False, [1, 2, 3])

    verify_to_nums_error_matches(
        "empty",
        "expected property 'empty' on / in .* to be assigned with " +
        re.escape("'empty = < (number) (number) ... >;', not 'empty;'"))
    verify_to_nums_error_matches(
        "string",
        "expected property 'string' on / in .* to be assigned with " +
        re.escape("'string = < (number) (number) ... >;', ") +
        re.escape("not 'string = \"foo\\tbar baz\";'"))

    # Test Property.to_bytes()

    def verify_to_bytes(prop, expected):
        actual = dt.root.props[prop].to_bytes()
        assert actual == expected, f"'{prop}' gives the wrong bytes"

    def verify_to_bytes_error_matches(prop, expected_re):
        with pytest.raises(dtlib.DTError) as e:
            dt.root.props[prop].to_bytes()
        assert re.fullmatch(expected_re, str(e.value)), \
            f"'{prop}' gives the wrong error"

    verify_to_bytes("u8", b"\x01")
    verify_to_bytes("bytes", b"\x01\x02\x03")

    verify_to_bytes_error_matches(
        "u16",
        "expected property 'u16' on / in .* to be assigned with " +
        re.escape("'u16 = [ (byte) (byte) ... ];', ") +
        re.escape("not 'u16 = /bits/ 16 < 0x1 0x2 >;'"))
    verify_to_bytes_error_matches(
        "empty",
        "expected property 'empty' on / in .* to be assigned with " +
        re.escape("'empty = [ (byte) (byte) ... ];', not 'empty;'"))

    # Test Property.to_string()

    def verify_to_string(prop, expected):
        actual = dt.root.props[prop].to_string()
        assert actual == expected, f"'{prop}' to_string gives the wrong string"

    def verify_to_string_error_matches(prop, expected_re):
        with pytest.raises(dtlib.DTError) as e:
            dt.root.props[prop].to_string()
        assert re.fullmatch(expected_re, str(e.value)), \
            f"'{prop}' gives the wrong error"

    verify_to_string("empty_string", "")
    verify_to_string("string", "foo\tbar baz")

    verify_to_string_error_matches(
        "u",
        "expected property 'u' on / in .* to be assigned with " +
        re.escape("'u = \"string\";', not 'u = < 0x1 >;'"))
    verify_to_string_error_matches(
        "strings",
        "expected property 'strings' on / in .* to be assigned with " +
        re.escape("'strings = \"string\";', ")+
        re.escape("not 'strings = \"foo\", \"bar\", \"baz\";'"))
    verify_to_string_error_matches(
        "invalid_string",
        re.escape(r"value of property 'invalid_string' (b'\xff\x00') on / ") +
        "in .* is not valid UTF-8")

    # Test Property.to_strings()

    def verify_to_strings(prop, expected):
        actual = dt.root.props[prop].to_strings()
        assert actual == expected, f"'{prop}' to_strings gives the wrong value"

    def verify_to_strings_error_matches(prop, expected_re):
        with pytest.raises(dtlib.DTError) as e:
            dt.root.props[prop].to_strings()
        assert re.fullmatch(expected_re, str(e.value)), \
            f"'{prop}' gives the wrong error"

    verify_to_strings("empty_string", [""])
    verify_to_strings("string", ["foo\tbar baz"])
    verify_to_strings("strings", ["foo", "bar", "baz"])

    verify_to_strings_error_matches(
        "u",
        "expected property 'u' on / in .* to be assigned with " +
        re.escape("'u = \"string\", \"string\", ... ;', not 'u = < 0x1 >;'"))
    verify_to_strings_error_matches(
        "invalid_strings",
        "value of property 'invalid_strings' " +
        re.escape(r"(b'foo\x00\xff\x00bar\x00') on / in ") +
        ".* is not valid UTF-8")

    # Test Property.to_node()

    def verify_to_node(prop, path):
        actual = dt.root.props[prop].to_node().path
        assert actual == path, f"'{prop}' points at wrong path"

    def verify_to_node_error_matches(prop, expected_re):
        with pytest.raises(dtlib.DTError) as e:
            dt.root.props[prop].to_node()
        assert re.fullmatch(expected_re, str(e.value)), \
            f"'{prop} gives the wrong error"

    verify_to_node("ref", "/target")

    verify_to_node_error_matches(
        "u",
        "expected property 'u' on / in .* to be assigned with " +
        re.escape("'u = < &foo >;', not 'u = < 0x1 >;'"))
    verify_to_node_error_matches(
        "string",
        "expected property 'string' on / in .* to be assigned with " +
        re.escape("'string = < &foo >;', not 'string = \"foo\\tbar baz\";'"))

    # Test Property.to_nodes()

    def verify_to_nodes(prop, paths):
        actual = [node.path for node in dt.root.props[prop].to_nodes()]
        assert actual == paths, f"'{prop} gives wrong node paths"

    def verify_to_nodes_error_matches(prop, expected_re):
        with pytest.raises(dtlib.DTError) as e:
            dt.root.props[prop].to_nodes()
        assert re.fullmatch(expected_re, str(e.value)), \
            f"'{prop} gives wrong error"

    verify_to_nodes("zero", [])
    verify_to_nodes("ref", ["/target"])
    verify_to_nodes("refs", ["/target", "/target2"])
    verify_to_nodes("refs2", ["/target", "/target2"])

    verify_to_nodes_error_matches(
        "u",
        "expected property 'u' on / in .* to be assigned with " +
        re.escape("'u = < &foo &bar ... >;', not 'u = < 0x1 >;'"))
    verify_to_nodes_error_matches(
        "string",
        "expected property 'string' on / in .* to be assigned with " +
        re.escape("'string = < &foo &bar ... >;', ") +
        re.escape("not 'string = \"foo\\tbar baz\";'"))

    # Test Property.to_path()

    def verify_to_path(prop, path):
        actual = dt.root.props[prop].to_path().path
        assert actual == path, f"'{prop} gives the wrong path"

    def verify_to_path_error_matches(prop, expected_re):
        with pytest.raises(dtlib.DTError) as e:
            dt.root.props[prop].to_path()
        assert re.fullmatch(expected_re, str(e.value)), \
            f"'{prop} gives the wrong error"

    verify_to_path("path", "/target")
    verify_to_path("manualpath", "/target")

    verify_to_path_error_matches(
        "u",
        "expected property 'u' on / in .* to be assigned with either " +
        re.escape("'u = &foo' or 'u = \"/path/to/node\"', not 'u = < 0x1 >;'"))
    verify_to_path_error_matches(
        "missingpath",
        "property 'missingpath' on / in .* points to the non-existent node "
        '"/missing"')

    # Test top-level to_num() and to_nums()

    def verify_raw_to_num(fn, prop, length, signed, expected):
        actual = fn(dt.root.props[prop].value, length, signed)
        assert actual == expected, \
            f"{fn.__name__}(<{prop}>, {length}, {signed}) gives wrong value"

    def verify_raw_to_num_error(fn, data, length, msg):
        with pytest.raises(dtlib.DTError) as e:
            fn(data, length)
        assert str(e.value) == msg, \
            (f"{fn.__name__}() called with data='{data}', length='{length}' "
             "gives the wrong error")

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

def test_duplicate_labels():
    '''
    It is an error to duplicate labels in most conditions, but there
    are some exceptions where it's OK.
    '''

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
    assert dt.memreserves == expected

    verify_error_endswith("""
/dts-v1/;

foo: / {
};
""",
":3 (column 6): parse error: expected /memreserve/ after labels at beginning of file")

def test_reprs():
    '''Test the __repr__() functions.'''

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

    assert re.fullmatch(r"DT\(filename='.*', include_path=\('foo', 'bar'\)\)",
                        repr(dt))
    assert re.fullmatch("<Property 'x' at '/' in '.*'>",
                        repr(dt.root.props["x"]))
    assert re.fullmatch("<Node /sub in '.*'>",
                        repr(dt.root.nodes["sub"]))

def test_names():
    '''Tests for node/property names.'''

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

    verify_error_endswith(r"""
/dts-v1/;

/ {
	foo@3;
};
""",
":4 (column 7): parse error: '@' is only allowed in node names")

    verify_error_endswith(r"""
/dts-v1/;

/ {
	foo@3 = < 0 >;
};
""",
":4 (column 8): parse error: '@' is only allowed in node names")

    verify_error_endswith(r"""
/dts-v1/;

/ {
	foo@2@3 {
	};
};
""",
":4 (column 10): parse error: multiple '@' in node name")

def test_dense_input():
    '''
    Test that a densely written DTS input round-trips to something
    readable.
    '''

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

def test_misc():
    '''Test miscellaneous errors and non-errors.'''

    verify_error_endswith("", ":1 (column 1): parse error: expected '/dts-v1/;' at start of file")

    verify_error_endswith("""
/dts-v1/;
""",
":2 (column 1): parse error: no root node defined")

    verify_error_endswith("""
/dts-v1/; /plugin/;
""",
":1 (column 11): parse error: /plugin/ is not supported")

    verify_error_endswith("""
/dts-v1/;

/ {
	foo: foo {
	};
};

// Only one label supported before label references at the top level
l1: l2: &foo {
};
""",
":9 (column 5): parse error: expected label reference (&foo)")

    verify_error_endswith("""
/dts-v1/;

/ {
        foo: {};
};
""",
":4 (column 14): parse error: expected node or property name")

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
