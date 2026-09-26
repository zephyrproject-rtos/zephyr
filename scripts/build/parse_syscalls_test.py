#!/usr/bin/env python3

# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
"""
Tests for parse_syscalls
"""

import io
import os
import tempfile
import unittest
from contextlib import redirect_stderr
from unittest import mock

import parse_syscalls

NOT_UTF8 = b'__syscall int broken(void);\n\xff\xfe\n'


class TestSyscallRegex(unittest.TestCase):
    """Tests for what counts as a system call declaration."""

    def find(self, text):
        # Not stripped: what the groups hold is what ends up in the json, so a
        # space the pattern was meant to swallow has to show up here.
        return [tuple(mo.groups()) for mo in parse_syscalls.syscall_regex.finditer(text)]

    def test_plain_syscall(self):
        self.assertEqual(
            self.find('__syscall int k_thing_get(struct k_thing *t);'),
            [('int k_thing_get', 'struct k_thing *t')],
        )

    def test_always_inline_syscall(self):
        """The longer spelling of the attribute is matched as well."""
        self.assertEqual(
            self.find('__syscall_always_inline void k_quick(void);'),
            [('void k_quick', 'void')],
        )

    def test_whitespace_before_the_parenthesis(self):
        self.assertEqual(
            self.find('__syscall int k_spaced (int a);'),
            [('int k_spaced', 'int a')],
        )

    def test_declaration_spread_over_lines(self):
        text = '__syscall int k_wrapped(\n\tint a,\n\tint b);'
        self.assertEqual(self.find(text), [('int k_wrapped', '\n\tint a,\n\tint b')])

    def test_something_that_is_not_a_syscall(self):
        self.assertEqual(self.find('int plain_function(void);'), [])


class TestTaggedStructs(unittest.TestCase):
    """Tests for the tags that mark a struct as a subsystem or a socket."""

    def test_subsystem_is_collected(self):
        """What is kept is the name, without the struct keyword."""
        found = []
        parse_syscalls.tagged_struct_update(
            found, '__subsystem', '__subsystem struct sensor_driver_api {\n};\n'
        )
        self.assertEqual(found, ['sensor_driver_api'])

    def test_net_socket_is_collected(self):
        found = []
        parse_syscalls.tagged_struct_update(
            found, '__net_socket', '__net_socket struct net_context {\n};\n'
        )
        self.assertEqual(found, ['net_context'])

    def test_an_untagged_struct_is_not_collected(self):
        found = []
        parse_syscalls.tagged_struct_update(found, '__subsystem', 'struct plain_struct {\n};\n')
        self.assertEqual(found, [])

    def test_a_tag_on_something_that_is_not_a_struct_is_not_collected(self):
        """The tag alone is not enough; the struct keyword has to follow it."""
        found = []
        parse_syscalls.tagged_struct_update(found, '__subsystem', '__subsystem thing {\n};\n')
        self.assertEqual(found, [])


class TestApiExtends(unittest.TestCase):
    """Tests for DEVICE_API_EXTENDS and how it is folded into the tag list."""

    def test_extends_is_read(self):
        extends = {}
        parse_syscalls.api_extends_update(extends, 'DEVICE_API_EXTENDS(child, parent, member);')
        self.assertEqual(extends, {'child_driver_api': 'parent_driver_api'})

    def test_a_file_without_the_macro_is_left_alone(self):
        extends = {}
        parse_syscalls.api_extends_update(extends, 'int unrelated(void);')
        self.assertEqual(extends, {})

    def test_merge_replaces_only_what_is_known(self):
        tagged = ['child_driver_api', 'other_driver_api']
        parse_syscalls.merge_extends_into_tagged(tagged, {'child_driver_api': 'parent_driver_api'})
        self.assertEqual(
            tagged,
            [
                {'name': 'child_driver_api', 'extends': 'parent_driver_api'},
                'other_driver_api',
            ],
        )


class TestAnalyzeHeaders(unittest.TestCase):
    """Tests for walking the headers and reading them."""

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.dir = self.tmp.name
        patcher = mock.patch.object(
            parse_syscalls, 'args', mock.Mock(emit_all_syscalls=False), create=True
        )
        patcher.start()
        self.addCleanup(patcher.stop)

    def write(self, name, text):
        path = os.path.join(self.dir, name)
        with open(path, 'w', encoding='utf-8') as fp:
            fp.write(text)
        return path

    def write_bytes(self, name, data):
        path = os.path.join(self.dir, name)
        with open(path, 'wb') as fp:
            fp.write(data)
        return path

    def file_list(self, *paths):
        path = os.path.join(self.dir, 'files.txt')
        with open(path, 'w', encoding='utf-8') as fp:
            fp.write(';'.join(paths))
        return path

    def test_a_header_is_read(self):
        self.write('k_thing.h', '__syscall int k_thing_get(int a);\n')

        syscalls, tagged = parse_syscalls.analyze_headers([self.dir], None, None)

        self.assertEqual(syscalls, [(['int k_thing_get', 'int a'], 'k_thing.h', True)])
        self.assertEqual(tagged, {'__subsystem': [], '__net_socket': []})

    def test_a_declaration_spread_over_lines_is_collapsed(self):
        """Whatever the header's layout, the json gets one line of it."""
        self.write('k_wrapped.h', '__syscall int k_wrapped(\n\tint a,\n\tint b);\n')

        syscalls, _ = parse_syscalls.analyze_headers([self.dir], None, None)

        self.assertEqual(syscalls, [(['int k_wrapped', ' int a, int b'], 'k_wrapped.h', True)])

    def test_a_scanned_header_is_not_emitted(self):
        self.write('k_thing.h', '__syscall int k_thing_get(int a);\n')

        syscalls, _ = parse_syscalls.analyze_headers(None, [self.dir], None)

        self.assertEqual(syscalls[0][2], False)

    def test_a_file_that_will_not_decode_is_named(self):
        """The message has to name the file that failed, and nothing else."""
        bad = self.write_bytes('bad.h', NOT_UTF8)
        self.write('good.h', '/* nothing */\n')

        err = io.StringIO()
        with self.assertRaises(UnicodeDecodeError), redirect_stderr(err):
            parse_syscalls.analyze_headers([self.dir], None, self.file_list(bad))

        self.assertIn(bad, err.getvalue())
        self.assertNotIn('good.h', err.getvalue())

    def test_a_file_that_will_not_decode_without_any_directory(self):
        """Nothing was walked, so nothing is left over to name."""
        bad = self.write_bytes('bad.h', NOT_UTF8)

        err = io.StringIO()
        with self.assertRaises(UnicodeDecodeError), redirect_stderr(err):
            parse_syscalls.analyze_headers(None, None, self.file_list(bad))

        self.assertIn(bad, err.getvalue())


if __name__ == '__main__':
    unittest.main()
