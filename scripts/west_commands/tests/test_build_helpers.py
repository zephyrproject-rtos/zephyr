# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

'''Tests for picking a build directory.'''

import os

import pytest

from build_helpers import _resolve_build_dir, find_build_dir, is_zephyr_build

FMT = 'build/{source_dir}'


@pytest.fixture
def tree(tmp_path):
    '''work/app, work/app/build and other/app, all real directories.'''
    (tmp_path / 'work' / 'app' / 'build').mkdir(parents=True)
    (tmp_path / 'other' / 'app').mkdir(parents=True)
    return tmp_path


def resolve(cwd, source_dir, fmt=FMT, guess=False):
    return _resolve_build_dir(fmt, guess, str(cwd), source_dir=str(source_dir), board='qemu_x86')


def test_a_source_below_the_cwd_is_named_relative_to_it(tree):
    got = resolve(tree / 'work', tree / 'work' / 'app')

    assert got == 'build/app'


def test_a_cwd_below_the_source_leaves_the_source_out(tree):
    """There is no useful way to name a directory above the current one."""
    got = resolve(tree / 'work' / 'app' / 'build', tree / 'work' / 'app')

    assert got == 'build/'


def test_a_source_beside_the_cwd_is_still_named_relative_to_it(tree):
    """Going up and across is a relative path like any other."""
    got = resolve(tree / 'work', tree / 'other' / 'app')

    assert got == 'build/' + os.path.join('..', 'other', 'app')


@pytest.mark.skipif(os.name != 'nt', reason='drive letters are a Windows thing')
def test_a_source_on_another_drive_leaves_the_source_out(tree):
    """os.path.relpath refuses two paths on different drives.

    The source directory is made relative before the format string is even
    looked at, so a format that does not mention it is affected too.
    """
    other_drive = 'D:' if str(tree)[0].upper() != 'D' else 'E:'

    assert resolve(tree / 'work', other_drive + os.sep + 'app') == 'build/'
    assert (
        _resolve_build_dir(
            'build',
            False,
            str(tree / 'work'),
            source_dir=other_drive + os.sep + 'app',
            board='qemu_x86',
        )
        == 'build'
    )


def test_a_source_that_is_not_named_at_all(tree):
    got = _resolve_build_dir('build', False, str(tree), board='qemu_x86')

    assert got == 'build'


def test_a_format_naming_something_that_was_not_given(tree):
    """Without --guess there is nothing to fall back on."""
    got = _resolve_build_dir('build/{shield}', False, str(tree), board='qemu_x86')

    assert got is None


def test_is_zephyr_build_of_a_directory_that_is_not_one(tree):
    assert is_zephyr_build(str(tree / 'work')) is False


def test_is_zephyr_build_of_nothing(tree):
    assert is_zephyr_build('') is False
    assert is_zephyr_build(None) is False


@pytest.mark.parametrize('variable', ['ZEPHYR_BASE', 'ZEPHYR_TOOLCHAIN_VARIANT'])
def test_is_zephyr_build_of_a_build_directory(tree, variable):
    """Either variable in the cache is enough, for old build directories."""
    build = tree / 'work' / 'app' / 'build'
    (build / 'CMakeCache.txt').write_text(f'{variable}:PATH=/somewhere\n', encoding='utf-8')

    assert is_zephyr_build(str(build)) is True


class FakeConfig:
    '''Just enough of west.configuration.Configuration to answer a lookup.'''

    def __init__(self, **values):
        self._values = values

    def get(self, key, default=None):
        return self._values.get(key, default)


def test_find_build_dir_prefers_what_it_is_given(tree):
    '''A directory named outright wins over build.dir-fmt.

    The format below points at a real build directory, so it is what would
    be returned if the named one did not come first. The config is handed
    in rather than left as None: None sends find_build_dir looking for a
    west workspace, and it would find whichever one the tests are run in.
    '''
    build = tree / 'work' / 'app' / 'build'
    (build / 'CMakeCache.txt').write_text('ZEPHYR_BASE:PATH=/somewhere\n', encoding='utf-8')
    config = FakeConfig(**{'build.dir-fmt': str(build)})

    assert find_build_dir(str(tree / 'work'), config=config) == str(tree / 'work')
    assert is_zephyr_build(str(build)) is True
