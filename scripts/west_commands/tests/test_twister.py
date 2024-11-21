# Copyright (c) 2023 Google Inc
#
# SPDX-License-Identifier: Apache-2.0

import argparse
from argparse import Namespace

import pytest
from twister_cmd import Twister

TEST_CASES = [
    {
        "r": [],
        "c": False,
        "test_only": False,
    },
    {
        "r": ["-c", "-T tests/ztest/base"],
        "c": True,
        "T": [" tests/ztest/base"],
        "test_only": False,
    },
    {
        "r": ["--test-only"],
        "c": False,
        "test_only": True,
    },
]

ARGS = Namespace(
    help=None,
    zephyr_base=None,
    verbose=0,
    command="twister",
)


@pytest.mark.parametrize("test_case", TEST_CASES)
def test_parse_remainder(test_case):
    twister = Twister()
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False
    )
    sub_p = parser.add_subparsers()

    twister.parser = twister.do_add_parser(sub_p)
    options = twister._parse_arguments(args=test_case["r"], options=None)

    assert options.clobber_output == test_case["c"]
    assert options.test_only == test_case["test_only"]
    if "T" in test_case:
        assert options.testsuite_root == test_case["T"]
