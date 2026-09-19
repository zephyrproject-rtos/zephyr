# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

'''Tests for reading a CMakeCache.txt.'''

import pytest

from zcmake import CMakeCache, CMakeCacheEntry, make_c_identifier


def write_cache(tmp_path, text):
    path = tmp_path / 'CMakeCache.txt'
    path.write_text(text, encoding='utf-8')
    return path


TESTDATA_C_IDENTIFIER = [
    ('already_fine', 'already_fine'),
    ('has-dashes', 'has_dashes'),
    ('has.dots', 'has_dots'),
    ('has spaces', 'has_spaces'),
    # A name that cannot start an identifier gains a leading underscore, and
    # keeps the character that made it so.
    ('1starts_with_a_digit', '_1starts_with_a_digit'),
    ('-starts_with_a_dash', '__starts_with_a_dash'),
    ('_underscore_is_fine', '_underscore_is_fine'),
]


@pytest.mark.parametrize('string, expected', TESTDATA_C_IDENTIFIER)
def test_make_c_identifier(string, expected):
    assert make_c_identifier(string) == expected


TESTDATA_NOT_AN_ENTRY = [
    '// a comment\n',
    '# another comment\n',
    # A comment that would otherwise read as an entry. The marker has to be
    # looked for before the pattern is tried, not instead of it.
    '// BOARD:STRING=qemu_x86\n',
    '# BOARD:STRING=qemu_x86\n',
    '\n',
    '   \n',
    'no type here=value\n',
    'BAD:NOSUCHTYPE=value\n',
]


@pytest.mark.parametrize('line', TESTDATA_NOT_AN_ENTRY)
def test_lines_that_are_not_entries(line):
    assert CMakeCacheEntry.from_line(line, 1) is None


TESTDATA_TYPED = [
    ('P:FILEPATH=/some/where\n', 'P', '/some/where'),
    ('P:PATH=/some/where\n', 'P', '/some/where'),
    ('S:STRING=plain\n', 'S', 'plain'),
    # ';' makes a list, but only for the types that say so.
    ('S:STRING=a;b;c\n', 'S', ['a', 'b', 'c']),
    ('I:INTERNAL=a;b\n', 'I', ['a', 'b']),
    ('T:STATIC=a;b\n', 'T', ['a', 'b']),
    ('U:UNINITIALIZED=a;b\n', 'U', ['a', 'b']),
    ('P:FILEPATH=a;b\n', 'P', 'a;b'),
    ('P:PATH=a;b\n', 'P', 'a;b'),
    # A name may contain a colon: the name match is non-greedy, so the type is
    # taken at the first colon that a type and an '=' follow, not the last --
    # which is what the second row below is for.
    ('A:B:PATH=/x\n', 'A:B', '/x'),
    ('A:PATH=x:PATH=y\n', 'A', 'x:PATH=y'),
    # An empty value stays empty.
    ('E:STRING=\n', 'E', ''),
]


@pytest.mark.parametrize('line, name, value', TESTDATA_TYPED)
def test_typed_entries(line, name, value):
    entry = CMakeCacheEntry.from_line(line, 1)

    assert entry.name == name
    assert entry.value == value


TESTDATA_BOOL = [
    ('ON', True),
    ('YES', True),
    ('TRUE', True),
    ('Y', True),
    ('on', True),
    ('True', True),
    ('OFF', False),
    ('NO', False),
    ('FALSE', False),
    ('N', False),
    ('IGNORE', False),
    ('NOTFOUND', False),
    ('', False),
    ('off', False),
    # Anything ending in -NOTFOUND is false, whatever comes before it.
    ('Foo-NOTFOUND', False),
    ('/usr/lib/libz.so-NOTFOUND', False),
    # "or a non-zero number", per the CMake rules quoted in _to_bool.
    ('1', True),
    ('2', True),
    ('-1', True),
    ('0', False),
]


@pytest.mark.parametrize('value, expected', TESTDATA_BOOL)
def test_bool_entries(value, expected):
    entry = CMakeCacheEntry.from_line(f'B:BOOL={value}\n', 1)

    assert entry.value is expected


def test_a_bool_that_is_none_of_those_is_refused():
    with pytest.raises(ValueError):
        CMakeCacheEntry.from_line('B:BOOL=maybe\n', 1)


TESTDATA_LINE_NUMBERS = [(1, 1), (2, 2), (7, 7)]


@pytest.mark.parametrize('lineno, reported', TESTDATA_LINE_NUMBERS)
def test_a_bad_bool_names_the_line_it_is_on(tmp_path, lineno, reported):
    '''The first line of a file is line 1, as it is everywhere else.'''
    # Not every filler line is an entry: a real CMakeCache.txt opens with a
    # comment banner and a blank line. Numbering the entries instead of the
    # lines would come out lower than 'reported' for every case but the
    # first, so the two cannot be confused for one another here.
    lines = [('// a comment', '', f'FILLER{i}:STRING=x')[i % 3] for i in range(lineno - 1)]
    lines.append('BAD:BOOL=maybe')
    path = write_cache(tmp_path, '\n'.join(lines) + '\n')

    with pytest.raises(ValueError) as excinfo:
        CMakeCache(path)

    assert f'on line {reported}:' in str(excinfo.value)


def test_cache_reads_a_file(tmp_path):
    path = write_cache(
        tmp_path,
        '\n'.join(
            [
                '// a comment',
                '',
                'BOARD:STRING=qemu_x86',
                'CONFIG_FOO:BOOL=ON',
                'SEARCH:STRING=a;b',
                'CMAKE_C_COMPILER:FILEPATH=/usr/bin/cc',
            ]
        )
        + '\n',
    )

    cache = CMakeCache(path)

    assert cache.get('BOARD') == 'qemu_x86'
    assert cache.get('CONFIG_FOO') is True
    assert cache.get('SEARCH') == ['a', 'b']
    assert cache.get('CMAKE_C_COMPILER') == '/usr/bin/cc'
    assert cache.get('NOT_THERE') is None
    assert cache.get('NOT_THERE', 'fallback') == 'fallback'


def test_cache_membership_and_item_access(tmp_path):
    path = write_cache(tmp_path, 'BOARD:STRING=qemu_x86\n')
    cache = CMakeCache(path)

    assert 'BOARD' in cache
    assert 'NOT_THERE' not in cache
    assert cache['BOARD'] == 'qemu_x86'
    with pytest.raises(KeyError):
        _ = cache['NOT_THERE']

    cache['ADDED'] = CMakeCacheEntry('ADDED', 'value')
    assert cache['ADDED'] == 'value'

    del cache['ADDED']
    assert 'ADDED' not in cache


def test_setting_something_that_is_not_an_entry_is_refused(tmp_path):
    cache = CMakeCache(write_cache(tmp_path, 'BOARD:STRING=qemu_x86\n'))

    with pytest.raises(TypeError):
        cache['ADDED'] = 'a bare string'


def test_iterating_a_cache_yields_entries(tmp_path):
    path = write_cache(tmp_path, 'A:STRING=1\nB:STRING=2\n')

    assert [entry.name for entry in CMakeCache(path)] == ['A', 'B']


TESTDATA_GET_LIST = [
    ('L:STRING=a;b', 'L', ['a', 'b']),
    ('L:STRING=only', 'L', ['only']),
    # An empty string is no elements, not one empty element.
    ('L:STRING=', 'L', []),
]


@pytest.mark.parametrize('line, name, expected', TESTDATA_GET_LIST)
def test_get_list(tmp_path, line, name, expected):
    cache = CMakeCache(write_cache(tmp_path, line + '\n'))

    assert cache.get_list(name) == expected


def test_get_list_of_something_missing(tmp_path):
    cache = CMakeCache(write_cache(tmp_path, 'A:STRING=1\n'))

    assert cache.get_list('NOT_THERE') == []
    assert cache.get_list('NOT_THERE', ['fallback']) == ['fallback']


def test_get_list_of_a_bool_is_refused(tmp_path):
    '''A BOOL is not a list of anything; asking for one is a mistake.'''
    cache = CMakeCache(write_cache(tmp_path, 'B:BOOL=ON\n'))

    with pytest.raises(RuntimeError):
        cache.get_list('B')
