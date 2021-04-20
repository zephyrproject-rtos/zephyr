# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from argparse import Namespace

from build import Build
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
