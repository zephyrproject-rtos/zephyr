# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from argparse import Namespace

from build import Build
from build_helpers import DEFAULT_BUILD_DIR
from pathlib import Path
import configparser
import os
import pytest

TEST_CASES = [
    {'r': [],
     's': None, 'c': None},
    {'r': ['source_dir'],
     's': 'source_dir', 'c': None},
    {'r': ['source_dir', '--'],
     's': 'source_dir', 'c': None},
    {'r': ['source_dir', '--', 'cmake_opt'],
     's': 'source_dir', 'c': ['cmake_opt']},
    {'r': ['source_dir', '--', 'cmake_opt', 'cmake_opt2'],
     's': 'source_dir', 'c': ['cmake_opt', 'cmake_opt2']},
    {'r': ['thing_one', 'thing_two'],
     's': 'thing_one', 'c': ['thing_two']},
    {'r': ['thing_one', 'thing_two', 'thing_three'],
     's': 'thing_one', 'c': ['thing_two', 'thing_three']},
    {'r': ['--'],
     's': None, 'c': None},
    {'r': ['--', '--'],
     's': None, 'c': ['--']},
    {'r': ['--', 'cmake_opt'],
     's': None, 'c': ['cmake_opt']},
    {'r': ['--', 'cmake_opt', 'cmake_opt2'],
     's': None, 'c': ['cmake_opt', 'cmake_opt2']},
    {'r': ['--', 'cmake_opt', 'cmake_opt2', '--'],
     's': None, 'c': ['cmake_opt', 'cmake_opt2', '--']},
    {'r': ['--', 'cmake_opt', 'cmake_opt2', '--', 'tool_opt'],
     's': None, 'c': ['cmake_opt', 'cmake_opt2', '--', 'tool_opt']},
    {'r': ['--', 'cmake_opt', 'cmake_opt2', '--', 'tool_opt', 'tool_opt2'],
     's': None, 'c': ['cmake_opt', 'cmake_opt2', '--', 'tool_opt',
                      'tool_opt2']},
    {'r': ['--', 'cmake_opt', 'cmake_opt2', '--', 'tool_opt', 'tool_opt2',
           '--'],
     's': None, 'c': ['cmake_opt', 'cmake_opt2', '--', 'tool_opt', 'tool_opt2',
                      '--']},
    ]

ARGS = Namespace(board=None, build_dir=None, cmake=False, command='build',
                 force=False, help=None, target=None, verbose=3, version=False,
                 zephyr_base=None)

@pytest.mark.parametrize('test_case', TEST_CASES)
def test_parse_remainder(test_case):
    b = Build()

    b.args = Namespace()
    b._parse_remainder(test_case['r'])
    assert b.args.source_dir == test_case['s']
    assert b.args.cmake_opts == test_case['c']


cwd = Path(os.getcwd())

DEFAULT_ARGS = Namespace(
    build_dir=None,
    board='native_sim',
    source_dir=os.path.join("any", "project", "app")
)

BUILD_DIR_FMT_VALID = [
    # dir_fmt, expected
    ('resolvable-{board}', cwd / f'resolvable-{DEFAULT_ARGS.board}'),
    ('my-dir-fmt-build-folder', cwd / 'my-dir-fmt-build-folder'),
]

BUILD_DIR_FMT_INVALID = [
    'unresolvable-because-of-unknown-{board}',
    '{invalid}'
]


def mock_dir_fmt_config(monkeypatch, dir_fmt):
    config = configparser.ConfigParser()
    config.add_section("build")
    config.set("build", "dir-fmt", dir_fmt)
    monkeypatch.setattr("build_helpers.config", config)
    monkeypatch.setattr("build.config", config)

def mock_is_zephyr_build(monkeypatch, func):
    monkeypatch.setattr("build_helpers.is_zephyr_build", func)


@pytest.fixture
def build_instance(monkeypatch):
    """Create a Build instance with default args and patched helpers."""
    b = Build()
    b.args = DEFAULT_ARGS
    b.config_board = None

    # mock configparser read method to bypass configs during test
    monkeypatch.setattr(configparser.ConfigParser, "read", lambda self, filenames: None)
    # mock os.makedirs to avoid filesystem operations during test
    monkeypatch.setattr("os.makedirs", lambda *a, **kw: None)
    # mock is_zephyr_build to always return False
    monkeypatch.setattr("build_helpers.is_zephyr_build", lambda path: False)
    # mock os.environ to make tests independent from environment variables
    monkeypatch.setattr("os.environ", {})

    return b

def test_build_dir_fallback(monkeypatch, build_instance):
    """Test: Fallback to DEFAULT_BUILD_DIR if no build dirs exist"""
    b = build_instance
    b._setup_build_dir()
    assert Path(b.build_dir) == cwd / DEFAULT_BUILD_DIR


@pytest.mark.parametrize('test_case', BUILD_DIR_FMT_VALID)
def test_build_dir_fmt_is_used(monkeypatch, build_instance, test_case):
    """Test: build.dir-fmt is used if it is resolvable (no matter if it exists)"""
    dir_fmt, expected = test_case
    b = build_instance

    # mock the config
    mock_dir_fmt_config(monkeypatch, dir_fmt)

    b._setup_build_dir()
    assert Path(b.build_dir) == expected


@pytest.mark.parametrize('dir_fmt', BUILD_DIR_FMT_INVALID)
def test_build_dir_fmt_is_not_resolvable(monkeypatch, build_instance, dir_fmt):
    """Test: Fallback to DEFAULT_BUILD_DIR if build.dir-fmt is not resolvable."""
    b = build_instance
    b.args.board = None

    # mock the config
    mock_dir_fmt_config(monkeypatch, dir_fmt)

    b._setup_build_dir()
    assert Path(b.build_dir) == cwd / DEFAULT_BUILD_DIR


def test_dir_fmt_preferred_over_others(monkeypatch, build_instance):
    """Test: build.dir-fmt is preferred over cwd and DEFAULT_BUILD_DIR (if all exist)"""
    b = build_instance
    dir_fmt = "my-dir-fmt-build-folder"

    # mock the config and is_zephyr_build
    mock_dir_fmt_config(monkeypatch, dir_fmt)
    mock_is_zephyr_build(monkeypatch,
        lambda path: path in [dir_fmt, cwd, DEFAULT_BUILD_DIR])

    b._setup_build_dir()
    assert Path(b.build_dir) == cwd / dir_fmt

def test_non_existent_dir_fmt_preferred_over_others(monkeypatch, build_instance):
    """Test: build.dir-fmt is preferred over cwd and DEFAULT_BUILD_DIR (if it not exists)"""
    b = build_instance
    dir_fmt = "my-dir-fmt-build-folder"

    # mock the config and is_zephyr_build
    mock_dir_fmt_config(monkeypatch, dir_fmt)
    mock_is_zephyr_build(monkeypatch,
        lambda path: path in [cwd, DEFAULT_BUILD_DIR])

    b._setup_build_dir()
    assert Path(b.build_dir) == cwd / dir_fmt


def test_cwd_preferred_over_default_build_dir(monkeypatch, build_instance):
    """Test: cwd is preferred over DEFAULT_BUILD_DIR (if both exist)"""
    b = build_instance

    # mock is_zephyr_build
    mock_is_zephyr_build(monkeypatch,
        lambda path: path in [str(cwd), DEFAULT_BUILD_DIR])

    b._setup_build_dir()
    assert Path(b.build_dir) == cwd

@pytest.mark.parametrize('dir_fmt', BUILD_DIR_FMT_INVALID)
def test_cwd_preferred_over_non_resolvable_dir_fmt(monkeypatch, build_instance, dir_fmt):
    """Test: cwd is preferred over dir-fmt, if dir-fmt is not resolvable"""
    b = build_instance

    # mock the config and is_zephyr_build
    mock_dir_fmt_config(monkeypatch, dir_fmt)
    mock_is_zephyr_build(monkeypatch,
        lambda path: path in [str(cwd)])

    b._setup_build_dir()
    assert Path(b.build_dir) == cwd

@pytest.mark.parametrize('test_case', BUILD_DIR_FMT_VALID)
def test_cwd_preferred_over_non_existent_dir_fmt(monkeypatch, build_instance, test_case):
    """Test: cwd is preferred over build.dir-fmt if cwd is a build directory and dir_fmt not"""
    dir_fmt, _ = test_case
    b = build_instance

    # mock the config and is_zephyr_build
    mock_dir_fmt_config(monkeypatch, dir_fmt)
    mock_is_zephyr_build(monkeypatch,
        lambda path: path in [str(cwd)])

    b._setup_build_dir()
    assert Path(b.build_dir) == cwd
