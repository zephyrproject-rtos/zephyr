# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import pytest

from build_helpers import _resolve_build_dir

# source_dir's cwd-relative rewrite is no longer the responsibility of
# `_resolve_build_dir`; that logic lives in build.Build._get_dir_fmt_context's
# context object (see test_dir_fmt.py). This file only exercises the
# remaining surface: format_map substitution and the guess-mode fallback.

TEST_CASES_RESOLVE_BUILD_DIR = [
    # (fmt, context, expected)
    # simple string (no placeholders)
    ('simple/string', {}, 'simple/string'),
    # placeholder resolves directly from context
    ('{source_dir}', {'source_dir': 'subdir'}, 'subdir'),
    # source_dir not present in context -> KeyError -> guess fallback fails -> None
    ('{source_dir}', {}, None),
    # invalid format arg -> same path
    ('{invalid}', {}, None),
    # app not present in context -> same path
    ('{app}', {}, None),
]


@pytest.mark.parametrize('test_case', TEST_CASES_RESOLVE_BUILD_DIR)
def test_resolve_build_dir(test_case):
    fmt, context, expected = test_case

    # test both guess=True and guess=False
    for guess in [True, False]:
        actual = _resolve_build_dir(fmt=fmt, guess=guess, context=context)
        assert actual == expected
