# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import configparser
import copy
import os
from argparse import Namespace
from dataclasses import dataclass, field
from pathlib import Path

import pytest

from build import Build

ROOT = Path(Path.cwd().anchor)
TEST_CWD = ROOT / 'path' / 'to' / 'my' / 'current' / 'cwd'
TEST_CWD_RELATIVE_TO_ROOT = TEST_CWD.relative_to(ROOT)


def setup_test_build(monkeypatch, test_args=None):
    # mock configparser read method to keep tests independent from actual configs
    monkeypatch.setattr(configparser.ConfigParser, 'read', lambda self, filenames: None)
    # mock os.makedirs to be independent from actual filesystem
    monkeypatch.setattr('os.makedirs', lambda *a, **kw: None)
    # mock west_topdir so that tests run independent from user machine
    monkeypatch.setattr('build.west_topdir', lambda *a, **kw: str(west_topdir))
    # mock os.getcwd and Path.cwd to use TEST_CWD
    monkeypatch.setattr('os.getcwd', lambda *a, **kw: str(TEST_CWD))
    monkeypatch.setattr('pathlib.Path.cwd', lambda *a, **kw: TEST_CWD)
    # mock os.environ to ignore all environment variables during test
    monkeypatch.setattr('os.environ', {})
    # skip board normalization (covered by dedicated tests) so these cases
    # stay isolated from the zephyr tree and the west manifest.
    monkeypatch.setattr('build.Build._normalize_board', lambda self, board: board)

    # set up Build
    b = Build()

    # apply test args
    b.args = copy.copy(DEFAULT_TEST_ARGS)
    if test_args:
        for k, v in vars(test_args).items():
            setattr(b.args, k, v)

    return b


# Use a hardcoded west_topdir to ensure that the tests run independently
# from the actual user machine
west_topdir = ROOT / 'any' / 'west' / 'workspace'

DEFAULT_TEST_ARGS = Namespace(
    board=None, source_dir=west_topdir / 'subdir' / 'project' / 'app', build_dir=None
)

TEST_CASES_GET_DIR_FMT_CONTEXT = [
    # (test_args, source_dir, expected)
    # fallback to cwd in default case
    (
        {},
        None,
        {
            'board': None,
            'west_topdir': str(west_topdir),
            'app': TEST_CWD.name,
            'source_dir': str(TEST_CWD),
            'source_dir_workspace': str(TEST_CWD_RELATIVE_TO_ROOT),
        },
    ),
    # check for correct source_dir and source_dir_workspace (if inside west_topdir)
    (
        {},
        west_topdir / 'my' / 'project',
        {
            'board': None,
            'west_topdir': str(west_topdir),
            'app': 'project',
            'source_dir': str(west_topdir / 'my' / 'project'),
            'source_dir_workspace': str(Path('my') / 'project'),
        },
    ),
    # check for correct source_dir and source_dir_workspace (if outside west_topdir)
    (
        {},
        ROOT / 'path' / 'to' / 'my-project',
        {
            'board': None,
            'west_topdir': str(west_topdir),
            'app': 'my-project',
            'source_dir': str(ROOT / 'path' / 'to' / 'my-project'),
            'source_dir_workspace': str(Path('path') / 'to' / 'my-project'),
        },
    ),
    # check for correct board
    (
        Namespace(board='native_sim'),
        None,
        {
            'board': 'native_sim',
            'west_topdir': str(west_topdir),
            'app': TEST_CWD.name,
            'source_dir': str(TEST_CWD),
            'source_dir_workspace': str(TEST_CWD_RELATIVE_TO_ROOT),
        },
    ),
]


@pytest.mark.parametrize('test_case', TEST_CASES_GET_DIR_FMT_CONTEXT)
def test_get_dir_fmt_context(monkeypatch, test_case):
    # extract data from the test case
    test_args, source_dir, expected = test_case

    # set up and run _get_dir_fmt_context
    b = setup_test_build(monkeypatch, test_args)
    b.args.source_dir = source_dir
    actual = b._get_dir_fmt_context()
    assert expected == actual


TEST_CASES_BUILD_DIR = [
    # (config_build, test_args, expected)
    # default build directory if no args and dir-fmt are specified
    ({}, None, Path('build')),
    # build_dir from args should always be preferred (if it is specified)
    ({}, Namespace(build_dir='from-args'), Path('from-args')),
    ({'dir-fmt': 'from-dir-fmt'}, Namespace(build_dir='from-args'), Path('from-args')),
    # build_dir is determined by resolving dir-fmt format string
    # must be able to resolve a simple string
    ({'dir-fmt': 'from-dir-fmt'}, None, 'from-dir-fmt'),
    # must be able to resolve west_topdir
    ({'dir-fmt': '{west_topdir}/build'}, None, west_topdir / 'build'),
    # must be able to resolve app
    ({'dir-fmt': '{app}'}, None, 'app'),
    # must be able to resolve source_dir (when it is inside west workspace)
    (
        {'dir-fmt': '{source_dir}'},
        None,
        # source_dir resolves to relative path (relative to cwd), so build
        # directory is depending on cwd and ends up outside 'build'
        os.path.relpath(DEFAULT_TEST_ARGS.source_dir, TEST_CWD),
    ),
    # source_dir dir is outside west workspace, so it is absolute path
    (
        {'dir-fmt': '{source_dir}'},
        Namespace(source_dir=ROOT / 'outside' / 'living' / 'app'),
        os.path.relpath(ROOT / 'outside' / 'living' / 'app', TEST_CWD),
    ),
    # must be able to resolve source_dir_workspace (when source_dir is inside west workspace)
    ({'dir-fmt': '{source_dir_workspace}'}, None, Path('subdir') / 'project' / 'app'),
    # must be able to resolve source_dir_workspace (when source_dir is outside west workspace)
    (
        {'dir-fmt': 'build/{source_dir_workspace}'},
        Namespace(source_dir=ROOT / 'outside' / 'living' / 'app'),
        Path('build') / 'outside' / 'living' / 'app',
    ),
    # must be able to resolve board (must be specified)
    ({'dir-fmt': '{board}'}, Namespace(board='native_sim'), 'native_sim'),
]


@pytest.mark.parametrize('test_case', TEST_CASES_BUILD_DIR)
def test_dir_fmt(monkeypatch, test_case):
    # extract data from the test case
    config_build, test_args, expected = test_case

    # apply given config_build
    config = configparser.ConfigParser()
    config.add_section("build")
    for k, v in config_build.items():
        config.set('build', k, v)
    monkeypatch.setattr("build_helpers.config", config)
    monkeypatch.setattr("build.config", config)

    # set up and run _setup_build_dir
    b = setup_test_build(monkeypatch, test_args)
    b._setup_build_dir()

    # check for expected build-dir
    assert os.path.abspath(expected) == b.build_dir


# Lightweight stand-ins for list_boards.Board / Soc so tests do not need to
# load YAML from the zephyr tree. Only the fields consumed by
# Build._normalize_board are populated.
@dataclass
class _FakeSoc:
    name: str


@dataclass
class _FakeBoard:
    name: str
    socs: list = field(default_factory=list)


_SINGLE_SOC_BOARDS = {
    'plank': _FakeBoard('plank', socs=[_FakeSoc('soc1')]),
}
_MULTI_SOC_BOARDS = {
    'multi': _FakeBoard('multi', socs=[_FakeSoc('soc1'), _FakeSoc('soc2')]),
}


TEST_CASES_NORMALIZE_BOARD = [
    # (boards, input, expected)
    # Unknown board: left untouched so CMake reports the error.
    ({}, 'unknown', 'unknown'),
    # No auto-fill needed: fully qualified identifier already.
    (_SINGLE_SOC_BOARDS, 'plank/soc1/cpu', 'plank/soc1/cpu'),
    # SoC omitted entirely: single-SoC board gets the SoC filled in.
    (_SINGLE_SOC_BOARDS, 'plank', 'plank/soc1'),
    # Empty SoC slot (the #101819 case): '/cpu' becomes 'soc1/cpu'.
    (_SINGLE_SOC_BOARDS, 'plank//cpu', 'plank/soc1/cpu'),
    # Revision is preserved across normalization.
    (_SINGLE_SOC_BOARDS, 'plank@1.0.0//cpu', 'plank@1.0.0/soc1/cpu'),
    (_SINGLE_SOC_BOARDS, 'plank@1.0.0', 'plank@1.0.0/soc1'),
    # Multi-SoC board: no auto-fill, input passes through unchanged.
    (_MULTI_SOC_BOARDS, 'multi', 'multi'),
    (_MULTI_SOC_BOARDS, 'multi//cpu', 'multi//cpu'),
    # Non-empty qualifier that does not start with '/': no normalization.
    (_SINGLE_SOC_BOARDS, 'plank/soc1', 'plank/soc1'),
    # Empty / None input: returned as-is.
    ({}, None, None),
    ({}, '', ''),
]


@pytest.mark.parametrize('test_case', TEST_CASES_NORMALIZE_BOARD)
def test_normalize_board(monkeypatch, test_case):
    boards, board_in, expected = test_case

    monkeypatch.setattr('build.zephyr_module.parse_modules', lambda *a, **kw: [])
    monkeypatch.setattr('build.list_boards.find_v2_boards', lambda args: boards)

    b = Build()
    # Satisfy the manifest property without needing a real west workspace.
    b._manifest = object()
    assert b._normalize_board(board_in) == expected


def test_normalize_board_swallows_lookup_errors(monkeypatch):
    # Runtime errors from board lookup (e.g. missing manifest, broken
    # module metadata) must fall back to the raw input rather than abort
    # west build.
    def explode(args):
        raise RuntimeError('tree scan failed')

    monkeypatch.setattr('build.zephyr_module.parse_modules', lambda *a, **kw: [])
    monkeypatch.setattr('build.list_boards.find_v2_boards', explode)

    b = Build()
    b._manifest = object()
    assert b._normalize_board('plank//cpu') == 'plank//cpu'
