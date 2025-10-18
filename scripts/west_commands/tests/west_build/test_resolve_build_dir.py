# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from pathlib import Path

import pytest
from build_helpers import _resolve_build_dir

cwd = Path.cwd()
root = Path(cwd.anchor)
TEST_CWD = root / 'path' / 'to' / 'my' / 'current' / 'cwd'

TEST_CASES_RESOLVE_BUILD_DIR = [
    # (fmt, kwargs, expected)
    # simple string (no format string)
    ('simple/string', {}, 'simple/string'),
    # source_dir is inside cwd
    ('{source_dir}', {'source_dir': TEST_CWD / 'subdir'}, 'subdir'),
    # source_dir is outside cwd
    ('{source_dir}', {'source_dir': TEST_CWD / '..' / 'subdir'}, str(Path('..') / 'subdir')),
    # cwd is inside source dir
    ('{source_dir}', {'source_dir': TEST_CWD / '..'}, ''),
    # source dir not resolvable by default
    ('{source_dir}', {}, None),
    # invalid format arg
    ('{invalid}', {}, None),
    # app is defined by default
    ('{app}', {}, None),
]


@pytest.mark.parametrize('test_case', TEST_CASES_RESOLVE_BUILD_DIR)
def test_resolve_build_dir(test_case):
    fmt, kwargs, expected = test_case

    # test both guess=True and guess=False
    for guess in [True, False]:
        actual = _resolve_build_dir(cwd=TEST_CWD, guess=guess, fmt=fmt, **kwargs)
        assert actual == expected
