#!/usr/bin/env python3
# Copyright (c) 2023 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

'''
This test file contains tests for platform.py module of twister
'''
import sys
import os
import mock
import pytest

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/pylib/twister"))

from twisterlib.platform import Platform


TESTDATA_1 = [
    (
"""\
identifier: dummy empty
arch: arch0
""",
        {
            'name': 'dummy empty',
            'arch': 'arch0',
            'twister': True,
            'ram': 128,
            'timeout_multiplier': 1.0,
            'ignore_tags': [],
            'only_tags': [],
            'default': False,
            'binaries': [],
            'flash': 512,
            'supported': set(),
            'vendor': '',
            'tier': -1,
            'type': 'na',
            'simulation': 'na',
            'simulation_exec': None,
            'supported_toolchains': [],
            'env': [],
            'env_satisfied': True
        },
        '<dummy empty on arch0>'
    ),
    (
"""\
identifier: dummy full
arch: riscv
twister: true
ram: 1024
testing:
  timeout_multiplier: 2.0
  ignore_tags:
    - tag1
    - tag2
  only_tags:
    - tag3
  default: true
  binaries:
    - dummy.exe
    - dummy.bin
flash: 4096
supported:
  - ble
  - netif:openthread
  - gpio
vendor: vendor1
tier: 1
type: unit
simulation: nsim
simulation_exec: nsimdrv
toolchain:
  - zephyr
  - llvm
env:
  - dummynonexistentvar
""",
        {
            'name': 'dummy full',
            'arch': 'riscv',
            'twister': True,
            'ram': 1024,
            'timeout_multiplier': 2.0,
            'ignore_tags': ['tag1', 'tag2'],
            'only_tags': ['tag3'],
            'default': True,
            'binaries': ['dummy.exe', 'dummy.bin'],
            'flash': 4096,
            'supported': set(['ble', 'netif', 'openthread', 'gpio']),
            'vendor': 'vendor1',
            'tier': 1,
            'type': 'unit',
            'simulation': 'nsim',
            'simulation_exec': 'nsimdrv',
            'supported_toolchains': ['zephyr', 'llvm', 'cross-compile'],
            'env': ['dummynonexistentvar'],
            'env_satisfied': False
        },
        '<dummy full on riscv>'
    ),
]

@pytest.mark.parametrize(
    'platform_text, expected_data, expected_repr',
    TESTDATA_1,
    ids=['almost empty specification', 'full specification']
)
def test_platform_load(platform_text, expected_data, expected_repr):
    platform = Platform()

    with mock.patch('builtins.open', mock.mock_open(read_data=platform_text)):
        platform.load('dummy.yaml')

    for k, v in expected_data.items():
        if not hasattr(platform, k):
            assert False, f'No key {k} in platform {platform}'
        att = getattr(platform, k)
        if isinstance(v, list) and not isinstance(att, list):
            assert False, f'Value mismatch in key {k} in platform {platform}'
        if isinstance(v, list):
            assert sorted(att) == sorted(v)
        else:
            assert att == v

    assert platform.__repr__() == expected_repr
