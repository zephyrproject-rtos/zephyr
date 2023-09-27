#!/usr/bin/env python3
# Copyright (c) 2023 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Tests for domains.py classes
"""

import mock
import os
import pytest
import sys

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/pylib/twister"))

import domains

from contextlib import nullcontext


TESTDATA_1 = [
    ('', False, 1, ['domains.yaml file not found: domains.yaml']),
    (
"""
default: some default
build_dir: my/dir
domains:
- name: some default
  build_dir: dir/2
- name: another
  build_dir: dir/3
flash_order: I don\'t think this is correct
""",
        True, 1, ['ERROR: Malformed yaml in file: domains.yaml']
    ),
    (
"""
default: None
build_dir: some/dir
""",
        True, None, []
    ),
]

@pytest.mark.parametrize(
    'f_contents, f_exists, exit_code, expected_logs',
    TESTDATA_1,
    ids=['no file', 'schema error', 'valid']
)
def test_from_file(caplog, f_contents, f_exists, exit_code, expected_logs):
    def mock_open(*args, **kwargs):
        if f_exists:
            return mock.mock_open(read_data=f_contents)(args, kwargs)
        raise FileNotFoundError(f'domains.yaml not found.')

    init_mock = mock.Mock(return_value=None)

    with mock.patch('domains.Domains.__init__', init_mock), \
         mock.patch('builtins.open', mock_open), \
         pytest.raises(SystemExit) if exit_code else nullcontext() as s_exit:
        result = domains.Domains.from_file('domains.yaml')

    if exit_code:
        assert str(s_exit.value) == str(exit_code)
    else:
        init_mock.assert_called_once()
        assert result is not None

    assert all([log in caplog.text for log in expected_logs])


TESTDATA_2 = [
    ({'build_dir': None, 'default': None}, True, None, [], None, {}),
    (
        {
            'build_dir': os.path.join('build', 'dir'),
            'domains': [
                {
                    'name': 'a domain',
                    'build_dir': os.path.join('dir', '1')
                },
                {
                    'name': 'default_domain',
                    'build_dir': os.path.join('dir', '2')
                }
            ],
            'default': 'default_domain',
            'flash_order': ['default_domain', 'a domain']
        },
        False,
        os.path.join('build', 'dir'),
        [('default_domain', os.path.join('dir', '2')),
         ('a domain', os.path.join('dir', '1'))],
        ('default_domain', os.path.join('dir', '2')),
        {'a domain': ('a domain', os.path.join('dir', '1')),
         'default_domain': ('default_domain', os.path.join('dir', '2'))}
    ),
]

@pytest.mark.parametrize(
    'data, expect_warning, expected_build_dir, expected_flash_order,' \
    ' expected_default, expected_domains',
    TESTDATA_2,
    ids=['required only', 'with default domain']
)
def test_from_data(
    caplog,
    data,
    expect_warning,
    expected_build_dir,
    expected_flash_order,
    expected_default,
    expected_domains
):
    def mock_domain(name, build_dir, *args, **kwargs):
        return name, build_dir

    warning_log = "no domains defined; this probably won't work"

    with mock.patch('domains.Domain', side_effect=mock_domain):
        doms = domains.Domains.from_data(data)

    if expect_warning:
        assert warning_log in caplog.text
    else:
        assert warning_log not in caplog.text

    assert doms.get_default_domain() == expected_default
    assert doms.get_top_build_dir() == expected_build_dir

    assert doms._domains == expected_domains

    assert all([d in expected_flash_order for d in doms._flash_order])
    assert all([d in doms._flash_order for d in expected_flash_order])


TESTDATA_3 = [
    (
        None,
        True,
        None,
        [],
        [('some', os.path.join('dir', '2')),
         ('order', os.path.join('dir', '1'))]
    ),
    (
        None,
        False,
        None,
        [],
        [('order', os.path.join('dir', '1')),
         ('some', os.path.join('dir', '2'))]
    ),
    (
        ['some', 'other'],
        False,
        1,
        ['domain other not found, valid domains are: order, some'],
        [('some', os.path.join('dir', '2')),
         ('order', os.path.join('dir', '1'))]
    ),
    (
        ['some'],
        False,
        None,
        [],
        [('some', os.path.join('dir', '2'))]
    ),
]

@pytest.mark.parametrize(
    'names, default_flash_order, exit_code, expected_logs, expected_result',
    TESTDATA_3,
    ids=['order only', 'no parameters', 'domain not found', 'valid']
)
def test_get_domains(
    caplog,
    names,
    default_flash_order,
    exit_code,
    expected_logs,
    expected_result
):
    doms = domains.Domains({'domains': [], 'default': None})
    doms._flash_order = [
        ('some', os.path.join('dir', '2')),
        ('order', os.path.join('dir', '1'))
    ]
    doms._domains = {
        'order': ('order', os.path.join('dir', '1')),
        'some': ('some', os.path.join('dir', '2'))
    }

    with pytest.raises(SystemExit) if exit_code else nullcontext() as s_exit:
        result = doms.get_domains(names, default_flash_order)

    assert all([log in caplog.text for log in expected_logs])

    if exit_code:
        assert str(s_exit.value) == str(exit_code)
    else:
        assert result == expected_result


def test_domain():
    name = 'Domain Name'
    build_dir = 'build/dir'

    domain = domains.Domain(name, build_dir)

    assert domain.name == name
    assert domain.build_dir == build_dir

    domain.name = 'New Name'
    domain.build_dir = 'new/dir'

    assert domain.name == 'New Name'
    assert domain.build_dir == 'new/dir'
