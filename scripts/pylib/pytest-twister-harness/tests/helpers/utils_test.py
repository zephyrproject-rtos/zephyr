# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import textwrap

import pytest
from twister_harness.helpers.utils import match_lines, match_no_lines

OUTPUT_LINES = textwrap.dedent("""\
    The Zen of Python, by Tim Peters
    Beautiful is better than ugly.
    Explicit is better than implicit.
    Simple is better than complex.\
""").split('\n')


CHECK_LINES = textwrap.dedent("""\
    Although never is often better than right now.
    If the implementation is hard to explain, it's a bad idea.
    If the implementation is easy to explain, it may be a good idea.
    Namespaces are one honking great idea -- let's do more of those!\
""").split('\n')


@pytest.fixture
def output_lines() -> list[str]:
    return OUTPUT_LINES


@pytest.mark.parametrize(
    "expected",
    [
        OUTPUT_LINES[0:1],  # one line
        OUTPUT_LINES[0:2],  # two lines
        OUTPUT_LINES[-1:],  # last line
        [OUTPUT_LINES[0][2:-2]],  # check partial of a text
    ],
)
def test_match_lines_positive(expected, output_lines):
    match_lines(output_lines, expected)


@pytest.mark.parametrize(
    "not_expected",
    [
        CHECK_LINES[0:1],
        CHECK_LINES[0:2],
        CHECK_LINES[-1:],
        [CHECK_LINES[-1][2:-2]],
        [CHECK_LINES[0], OUTPUT_LINES[0]],  # one line match, one not
        CHECK_LINES[::-1],  # reverted order
    ],
)
def test_match_lines_negative(not_expected, output_lines):
    with pytest.raises(AssertionError, match="Missing lines.*"):
        match_lines(output_lines, not_expected)


@pytest.mark.parametrize(
    "not_expected",
    [
        OUTPUT_LINES[0:1],
        OUTPUT_LINES[1:3],
        OUTPUT_LINES[-1:],
        [CHECK_LINES[0], OUTPUT_LINES[0]],  # one line match, one not
        OUTPUT_LINES[::-1],  # reverted order
    ],
)
def test_match_no_lines_negative(not_expected, output_lines):
    with pytest.raises(AssertionError, match="Found lines that should not be present.*"):
        match_no_lines(output_lines, not_expected)


@pytest.mark.parametrize(
    "expected",
    [
        CHECK_LINES[0:1],
        CHECK_LINES[3:5],
        CHECK_LINES[-1:],
    ],
)
def test_match_no_lines_positive(expected, output_lines):
    match_no_lines(output_lines, expected)
