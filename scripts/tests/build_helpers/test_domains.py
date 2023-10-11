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
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/pylib/build_helpers"))

import domains

from contextlib import nullcontext


TESTDATA_1 = [
    ('', False, 1, ['domains.yaml file not found: domains.yaml']),
    (
"""
default: None
build_dir: some/dir
domains: []
""",
        True, None, []
    ),
]

@pytest.mark.parametrize(
    'f_contents, f_exists, exit_code, expected_logs',
    TESTDATA_1,
    ids=['no file', 'valid']
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
        1, None, None, None, None
    ),
    (
"""
build_dir: build/dir
domains:
- name: a domain
  build_dir: dir/1
- name: default_domain
  build_dir: dir/2
default: default_domain
flash_order:
- default_domain
- a domain
""",
        None,
        'build/dir',
        [('default_domain', 'dir/2'), ('a domain', 'dir/1')],
        ('default_domain', 'dir/2'),
        {'a domain': ('a domain', 'dir/1'),
         'default_domain': ('default_domain', 'dir/2')}
    ),
]

@pytest.mark.parametrize(
    'data, exit_code, expected_build_dir, expected_flash_order,' \
    ' expected_default, expected_domains',
    TESTDATA_2,
    ids=['invalid', 'valid']
)
def test_from_yaml(
    caplog,
    data,
    exit_code,
    expected_build_dir,
    expected_flash_order,
    expected_default,
    expected_domains
):
    def mock_domain(name, build_dir, *args, **kwargs):
        return name, build_dir

    with mock.patch('domains.Domain', side_effect=mock_domain), \
         pytest.raises(SystemExit) if exit_code else nullcontext() as exit_st:
        doms = domains.Domains.from_yaml(data)

    if exit_code:
        assert str(exit_st.value) == str(exit_code)
        return

    assert doms.get_default_domain() == expected_default
    assert doms.get_top_build_dir() == expected_build_dir

    assert doms._domains == expected_domains

    assert all([d in expected_flash_order for d in doms._flash_order])
    assert all([d in doms._flash_order for d in expected_flash_order])


TESTDATA_3 = [
    (
        None,
        True,
        [('some', os.path.join('dir', '2')),
         ('order', os.path.join('dir', '1'))]
    ),
    (
        None,
        False,
        [('order', os.path.join('dir', '1')),
         ('some', os.path.join('dir', '2'))]
    ),
    (
        ['some'],
        False,
        [('some', os.path.join('dir', '2'))]
    ),
]

@pytest.mark.parametrize(
    'names, default_flash_order, expected_result',
    TESTDATA_3,
    ids=['order only', 'no parameters', 'valid']
)
def test_get_domains(
    caplog,
    names,
    default_flash_order,
    expected_result
):
    doms = domains.Domains(
"""
domains:
- name: dummy
  build_dir: dummy
default: dummy
build_dir: dummy
"""
    )
    doms._flash_order = [
        ('some', os.path.join('dir', '2')),
        ('order', os.path.join('dir', '1'))
    ]
    doms._domains = {
        'order': ('order', os.path.join('dir', '1')),
        'some': ('some', os.path.join('dir', '2'))
    }

    result = doms.get_domains(names, default_flash_order)

    assert result == expected_result



TESTDATA_3 = [
    (
        'other',
        1,
        ['domain "other" not found, valid domains are: order, some'],
        None
    ),
    (
        'some',
        None,
        [],
        ('some', os.path.join('dir', '2'))
    ),
]

@pytest.mark.parametrize(
    'name, exit_code, expected_logs, expected_result',
    TESTDATA_3,
    ids=['domain not found', 'valid']
)
def test_get_domain(
    caplog,
    name,
    exit_code,
    expected_logs,
    expected_result
):
    doms = domains.Domains(
"""
domains:
- name: dummy
  build_dir: dummy
default: dummy
build_dir: dummy
"""
    )
    doms._flash_order = [
        ('some', os.path.join('dir', '2')),
        ('order', os.path.join('dir', '1'))
    ]
    doms._domains = {
        'order': ('order', os.path.join('dir', '1')),
        'some': ('some', os.path.join('dir', '2'))
    }

    with pytest.raises(SystemExit) if exit_code else nullcontext() as s_exit:
        result = doms.get_domain(name)

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
