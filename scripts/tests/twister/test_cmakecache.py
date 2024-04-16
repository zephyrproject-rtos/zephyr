#!/usr/bin/env python3
# Copyright (c) 2023 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Tests for cmakecache.py classes' methods
"""

import mock
import pytest

from contextlib import nullcontext

from twisterlib.cmakecache import CMakeCacheEntry, CMakeCache


TESTDATA_1 = [
    ('ON', True),
    ('YES', True),
    ('TRUE', True),
    ('Y', True),
    ('OFF', False),
    ('NO', False),
    ('FALSE', False),
    ('N', False),
    ('IGNORE', False),
    ('NOTFOUND', False),
    ('', False),
    ('DUMMY-NOTFOUND', False),
    ('1', True),
    ('0', False),
    ('I AM NOT A PROPER VALUE', ValueError),
]


@pytest.mark.parametrize(
    'cmake_bool, expected_bool',
    TESTDATA_1,
    ids=[t[0] for t in TESTDATA_1]
)
def test_cmakecacheentry_to_bool(cmake_bool, expected_bool):
    with pytest.raises(expected_bool) if \
     not isinstance(expected_bool, bool) else nullcontext() as exception:
        b = CMakeCacheEntry._to_bool(cmake_bool)

    if exception is None:
        assert b == expected_bool
    else:
        assert str(exception.value) == f'invalid bool {cmake_bool}'


TESTDATA_2 = [
    (
        '// I am a comment',
        None
    ),
    (
        '# I am a comment too',
        None
    ),
    (
        '  	\r\n	     ',
        None
    ),
    (
        'DUMMY:WRONG_TYPE=???',
        None
    ),
    (
        'DUMMY_VALUE_NAME1:STRING=I am a dummy string',
        CMakeCacheEntry('DUMMY_VALUE_NAME1', 'I am a dummy string')
    ),
    (
       'DUMMY_VALUE_NAME2:STRING=list_el1;list_el2;list_el3',
        CMakeCacheEntry(
            'DUMMY_VALUE_NAME2',
            ['list_el1', 'list_el2', 'list_el3']
        )
    ),
    (
        'DUMMY_VALUE_NAME3:INTERNAL=I am a dummy internal string',
        CMakeCacheEntry('DUMMY_VALUE_NAME3', 'I am a dummy internal string')),
    (
        'DUMMY_VALUE_NAME4:INTERNAL=list_el1;list_el2',
        CMakeCacheEntry('DUMMY_VALUE_NAME4', ['list_el1', 'list_el2'])
    ),
    (
        'DUMMY_VALUE_NAME5:FILEPATH=/path/to/dir',
        CMakeCacheEntry('DUMMY_VALUE_NAME5', '/path/to/dir')
    ),
    (
        'DUMMY_VALUE_NAME6:PATH=/path/to/dir/file.txt',
        CMakeCacheEntry('DUMMY_VALUE_NAME6', '/path/to/dir/file.txt')),
    (
        'DUMMY_VALUE_NAME7:BOOL=Y',
        CMakeCacheEntry('DUMMY_VALUE_NAME7', True)
    ),
    (
        'DUMMY_VALUE_NAME8:BOOL=FALSE',
        CMakeCacheEntry('DUMMY_VALUE_NAME8', False)
    ),
    (
        'DUMMY_VALUE_NAME9:BOOL=NOT_A_BOOL',
        ValueError(
          (
              'invalid bool NOT_A_BOOL',
              'on line 7: DUMMY_VALUE_NAME9:BOOL=NOT_A_BOOL'
          )
        )
    ),
]


@pytest.mark.parametrize(
    'cmake_line, expected',
    TESTDATA_2,
    ids=[
        '// comment',
        '# comment',
        'whitespace',
        'unrecognised type',
        'string',
        'string list',
        'internal string',
        'internal list',
        'filepath',
        'path',
        'true bool',
        'false bool',
        'not a bool'
    ]
)
def test_cmakecacheentry_from_line(cmake_line, expected):
    cmake_line_number = 7

    with pytest.raises(type(expected)) if \
     isinstance(expected, Exception) else nullcontext() as exception:
        entry = CMakeCacheEntry.from_line(cmake_line, cmake_line_number)

    if exception is not None:
        assert repr(exception.value) == repr(expected)
        return

    if expected is None:
        assert entry is None
        return

    assert entry.name == expected.name
    assert entry.value == expected.value


TESTDATA_3 = [
    (
        CMakeCacheEntry('DUMMY_NAME1', 'dummy value'),
        'CMakeCacheEntry(name=DUMMY_NAME1, value=dummy value)'
    ),
    (
        CMakeCacheEntry('DUMMY_NAME2', False),
        'CMakeCacheEntry(name=DUMMY_NAME2, value=False)'
    )
]


@pytest.mark.parametrize(
    'cmake_cache_entry, expected',
    TESTDATA_3,
    ids=['string value', 'bool value']
)
def test_cmakecacheentry_str(cmake_cache_entry, expected):
    assert str(cmake_cache_entry) == expected


def test_cmakecache_load():
    file_data = (
        'DUMMY_NAME1:STRING=First line\n'
        '//Comment on the second line\n'
        'DUMMY_NAME2:STRING=Third line\n'
        'DUMMY_NAME3:STRING=Fourth line\n'
    )

    with mock.patch('builtins.open', mock.mock_open(read_data=file_data)):
        cache = CMakeCache.from_file('dummy/path/CMakeCache.txt')

    assert cache.cache_file == 'dummy/path/CMakeCache.txt'

    expected = [
        ('DUMMY_NAME1', 'First line'),
        ('DUMMY_NAME2', 'Third line'),
        ('DUMMY_NAME3', 'Fourth line')
    ]

    for expect in reversed(expected):
        item = cache._entries.popitem()

        assert item[0] == expect[0]
        assert item[1].name == expect[0]
        assert item[1].value == expect[1]


def test_cmakecache_get():
    file_data = 'DUMMY_NAME:STRING=Dummy value'

    with mock.patch('builtins.open', mock.mock_open(read_data=file_data)):
        cache = CMakeCache.from_file('dummy/path/CMakeCache.txt')

    good_val = cache.get('DUMMY_NAME')

    assert good_val == 'Dummy value'

    bad_val = cache.get('ANOTHER_NAME')

    assert bad_val is None

    bad_val = cache.get('ANOTHER_NAME', default='No such value')

    assert bad_val == 'No such value'


TESTDATA_4 = [
    ('STRING=el1;el2;el3;el4', True, ['el1', 'el2', 'el3', 'el4']),
    ('STRING=dummy value', True, ['dummy value']),
    ('STRING=', True, []),
    ('BOOL=True', True, RuntimeError),
    ('STRING=dummy value', False, []),
]


@pytest.mark.parametrize(
    'value, correct_get, expected',
    TESTDATA_4,
    ids=['list', 'single value', 'empty', 'exception', 'get failure']
)
def test_cmakecache_get_list(value, correct_get, expected):
    file_data = f'DUMMY_NAME:{value}'

    with mock.patch('builtins.open', mock.mock_open(read_data=file_data)):
        cache = CMakeCache.from_file('dummy/path/CMakeCache.txt')

    with pytest.raises(expected) if \
     isinstance(expected, type) and issubclass(expected, Exception) else \
     nullcontext() as exception:
        res = cache.get_list('DUMMY_NAME') if \
         correct_get else cache.get_list('ANOTHER_NAME')

    if exception is None:
        assert res == expected


def test_cmakecache_dunders():
    file_data = f'DUMMY_NAME:STRING=dummy value'

    with mock.patch('builtins.open', mock.mock_open(read_data=file_data)):
        cache = CMakeCache.from_file('dummy/path/CMakeCache.txt')

    assert len(list(cache.__iter__())) == 1

    cache.__setitem__(
        'ANOTHER_NAME',
        CMakeCacheEntry('ANOTHER_NAME', 'another value')
    )

    assert cache.__contains__('ANOTHER_NAME')
    assert cache.__getitem__('ANOTHER_NAME') == 'another value'

    cache.__delitem__('ANOTHER_NAME')

    assert not cache.__contains__('ANOTHER_NAME')

    assert len(list(cache.__iter__())) == 1

    with pytest.raises(TypeError):
        cache.__setitem__('WRONG_TYPE', 'Yet Another Dummy Value')
