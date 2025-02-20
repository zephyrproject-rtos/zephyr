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

from contextlib import nullcontext
from pykwalify.errors import SchemaError

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/pylib/twister"))

from twisterlib.platform import Platform, Simulator, generate_platforms


TESTDATA_1 = [
    (
"""\
identifier: dummy empty
arch: arc
""",
        {
            'name': 'dummy empty',
            'arch': 'arc',
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
            'simulators': [],
            'supported_toolchains': [],
            'env': [],
            'env_satisfied': True
        },
        '<dummy empty on arc>'
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
simulation:
- name: nsim
  exec: nsimdrv
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
            'simulators': [Simulator({'name': 'nsim', 'exec': 'nsimdrv'})],
            'supported_toolchains': ['zephyr', 'llvm', 'cross-compile'],
            'env': ['dummynonexistentvar'],
            'env_satisfied': False
        },
        '<dummy full on riscv>'
    ),
]

# This test is disabled because the Platform loading was changed significantly.
# The test should be updated to reflect the new implementation.

@pytest.mark.parametrize(
    'platform_text, expected_data, expected_repr',
    TESTDATA_1,
    ids=['almost empty specification', 'full specification']
)
def xtest_platform_load(platform_text, expected_data, expected_repr):
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


TESTDATA_2 = [
    (
        ['m0'],
        None,
        {
            'p1e1/s1', 'p1e2/s1', 'p2/s1', 'p3@A/s2/c1', 'p3@B/s2/c1',
        },
    ),
    (
        ['m0', 'm1'],
        None,
        {
            'p1e1/s1', 'p1e2/s1', 'p2/s1', 'p3@A/s2/c1', 'p3@B/s2/c1',
            'p1e1/s1/v1', 'p1e1/s1/v2', 'p1e2/s1/v1', 'p2/s1/v1',
        },
    ),
    (
        ['m0', 'm1', 'm2'],
        None,
        {
            'p1e1/s1', 'p1e2/s1', 'p2/s1', 'p3@A/s2/c1', 'p3@B/s2/c1',
            'p1e1/s1/v1', 'p1e1/s1/v2', 'p1e2/s1/v1', 'p2/s1/v1',
            'p3@A/s2/c2', 'p3@B/s2/c2', 'p4/s1',
        },
    ),
    (
        ['m0', 'm3'],
        Exception("Duplicate platform identifier p1e1/s1 found"),
        None,
    ),
    (
        ['m0', 'm1', 'm4'],
        Exception("Duplicate platform identifier p1e2/s1/v1 found"),
        None,
    ),
    (
        ['m0', 'm5'],
        SchemaError(), # Unknown message as this is raised externally
        None,
    ),
]

@pytest.mark.parametrize(
    'roots, expected_exception, expected_platform_names',
    TESTDATA_2,
    ids=[
        'default board root',
        '1 extra board root',
        '2 extra board roots',
        '1 extra board root, duplicate platform',
        '2 extra board roots, duplicate platform',
        '1 extra board root, malformed yaml',
    ]
)
def test_generate_platforms(
    tmp_path,
    roots,
    expected_exception,
    expected_platform_names,
):
    tmp_files = {
        'm0/boards/zephyr/p1/board.yml': """\
boards:
  - name: p1e1
    vendor: zephyr
    socs:
      - name: s1
  - name: p1e2
    vendor: zephyr
    socs:
      - name: s1
""",
        'm0/boards/zephyr/p1/twister.yaml': """\
type: native
arch: x86
variants:
  p1e1:
    twister: False
  p1e2:
    sysbuild: True
""",
        'm0/boards/zephyr/p2/board.yml': """\
boards:
  - name: p2
    vendor: zephyr
    socs:
      - name: s1
""",
        'm0/boards/zephyr/p2/p2.yaml': """\
identifier: p2/s1
type: sim
arch: x86
vendor: vendor2
testing:
  default: True
""",
        'm0/boards/arm/p3/board.yml': """\
board:
  name: p3
  vendor: arm
  revision:
    format: letter
    default: "A"
    revisions:
      - name: "A"
      - name: "B"
  socs:
    - name: s2
""",
        'm0/boards/arm/p3/twister.yaml': """\
type: unit
arch: arm
vendor: vendor3
sysbuild: True
variants:
  p3/s2/c1:
    testing:
      timeout_multiplier: 2.71828
  p3@B/s2/c1:
    testing:
      timeout_multiplier: 3.14159
""",
        'm0/soc/zephyr/soc.yml': """\
family:
  - name: zephyr
    series:
      - name: zephyr_testing
        socs:
          - name: s1
          - name: s2
            cpuclusters:
              - name: c1
""",
        'm1/boards/zephyr/p1e1/board.yml': """\
board:
  extend: p1e1
  variants:
    - name: v1
      qualifier: s1
    - name: v2
      qualifier: s1
""",
        'm1/boards/zephyr/p1e1/twister.yaml': """\
variants:
  p1e1/s1/v1:
    testing:
      default: True
""",
        'm1/boards/zephyr/p1e2/board.yml': """\
board:
  extend: p1e2
  variants:
    - name: v1
      qualifier: s1
""",
        'm1/boards/zephyr/p2/board.yml': """\
board:
  extend: p2
  variants:
    - name: v1
      qualifier: s1
""",
        'm1/boards/zephyr/p2/p2_s1_v1.yaml': """\
identifier: p2/s1/v1
""",
        'm2/boards/misc/board.yml': """\
boards:
  - extend: p3
  - name: p4
    vendor: misc
    socs:
      - name: s1
""",
        'm2/boards/misc/twister.yaml': """\
type: qemu
arch: riscv
vendor: vendor4
simulation:
  - name: qemu
variants:
  p3@A/s2/c2:
    sysbuild: False
""",
        'm2/soc/zephyr/soc.yml': """\
socs:
  - extend: s2
    cpuclusters:
      - name: c2
""",
        'm3/boards/zephyr/p1e1/board.yml': """\
board:
  extend: p1e1
""",
        'm3/boards/zephyr/p1e1/twister.yaml': """\
variants:
  p1e1/s1:
    name: Duplicate Platform
""",
        'm4/boards/zephyr/p1e2/board.yml': """\
board:
  extend: p2
""",
        'm4/boards/zephyr/p1e2/p1e2_s1_v1.yaml': """\
identifier: p1e2/s1/v1
""",
        'm5/boards/zephyr/p2/p2-2.yaml': """\
testing:
  ć#@%!#!#^#@%@:1.0
identifier: p2_2
type: sim
arch: x86
vendor: vendor2
""",
        'm5/boards/zephyr/p2/board.yml': """\
board:
  extend: p2
""",
    }

    for filename, content in tmp_files.items():
        (tmp_path / filename).parent.mkdir(parents=True, exist_ok=True)
        (tmp_path / filename).write_text(content)

    roots = list(map(tmp_path.joinpath, roots))
    with pytest.raises(type(expected_exception)) if \
          expected_exception else nullcontext() as exception:
        platforms = list(generate_platforms(board_roots=roots, soc_roots=roots, arch_roots=roots))

    if expected_exception:
        if expected_exception.args:
            assert str(expected_exception) == str(exception.value)
        return

    platform_names = {platform.name for platform in platforms}
    assert len(platforms) == len(platform_names)
    assert platform_names == expected_platform_names

    expected_data = {
        'p1e1/s1': {
            'aliases': ['p1e1/s1', 'p1e1'],
            # m0/boards/zephyr/p1/board.yml
            'vendor': 'zephyr',
            # m0/boards/zephyr/p1/twister.yaml (base + variant)
            'twister': False,
            'arch': 'x86',
            'type': 'native',
        },
        'p1e2/s1': {
            'aliases': ['p1e2/s1', 'p1e2'],
            # m0/boards/zephyr/p1/board.yml
            'vendor': 'zephyr',
            # m0/boards/zephyr/p1/twister.yaml (base + variant)
            'sysbuild': True,
            'arch': 'x86',
            'type': 'native',
        },
        'p1e1/s1/v1': {
            'aliases': ['p1e1/s1/v1'],
            # m0/boards/zephyr/p1/board.yml
            'vendor': 'zephyr',
            # m0/boards/zephyr/p1/twister.yaml (base)
            # m1/boards/zephyr/p1e1/twister.yaml (variant)
            'default': True,
            'arch': 'x86',
            'type': 'native',
        },
        'p1e1/s1/v2': {
            'aliases': ['p1e1/s1/v2'],
            # m0/boards/zephyr/p1/board.yml
            'vendor': 'zephyr',
            # m0/boards/zephyr/p1/twister.yaml (base)
            'arch': 'x86',
            'type': 'native',
        },
        'p1e2/s1/v1': {
            'aliases': ['p1e2/s1/v1'],
            # m0/boards/zephyr/p1/board.yml
            'vendor': 'zephyr',
            # m0/boards/zephyr/p1/twister.yaml (base)
            'arch': 'x86',
            'type': 'native',
        },
        'p2/s1': {
            'aliases': ['p2/s1', 'p2'],
            # m0/boards/zephyr/p2/board.yml
            'vendor': 'zephyr',
            # m0/boards/zephyr/p2/p2.yaml
            'default': True,
            'arch': 'x86',
            'type': 'sim',
        },
        'p2/s1/v1': {
            'aliases': ['p2/s1/v1'],
            # m0/boards/zephyr/p2/board.yml
            'vendor': 'zephyr',
            # m1/boards/zephyr/p2/p2_s1_v1.yaml
        },
        'p3@A/s2/c1': {
            'aliases': ['p3@A/s2/c1', 'p3/s2/c1'],
            # m0/boards/arm/p3/board.yml
            'vendor': 'arm',
            # m0/boards/arm/p3/twister.yaml (base + variant)
            'sysbuild': True,
            'timeout_multiplier': 2.71828,
            'arch': 'arm',
            'type': 'unit',
        },
        'p3@B/s2/c1': {
            'aliases': ['p3@B/s2/c1'],
            # m0/boards/arm/p3/board.yml
            'vendor': 'arm',
            # m0/boards/arm/p3/twister.yaml (base + variant)
            'sysbuild': True,
            'timeout_multiplier': 3.14159,
            'arch': 'arm',
            'type': 'unit',
        },
        'p3@A/s2/c2': {
            'aliases': ['p3@A/s2/c2', 'p3/s2/c2'],
            # m0/boards/arm/p3/board.yml
            'vendor': 'arm',
            # m0/boards/arm/p3/twister.yaml (base)
            # m2/boards/misc/twister.yaml (variant)
            'sysbuild': False,
            'arch': 'arm',
            'type': 'unit',
        },
        'p3@B/s2/c2': {
            'aliases': ['p3@B/s2/c2'],
            # m0/boards/arm/p3/board.yml
            'vendor': 'arm',
            # m0/boards/arm/p3/twister.yaml (base)
            'sysbuild': True,
            'arch': 'arm',
            'type': 'unit',
        },
        'p4/s1': {
            'aliases': ['p4/s1', 'p4'],
            # m2/boards/misc/board.yml
            'vendor': 'misc',
            # m2/boards/misc/twister.yaml (base)
            'arch': 'riscv',
            'type': 'qemu',
            'simulators': [Simulator({'name': 'qemu'})],
            'simulation': 'qemu',
        },
    }

    init_platform = Platform()
    for platform in platforms:
        expected_platform_data = expected_data[platform.name]
        for attr, default in vars(init_platform).items():
            if attr in {'name', 'normalized_name', 'supported_toolchains'}:
                continue
            expected = expected_platform_data.get(attr, default)
            actual = getattr(platform, attr, None)
            assert expected == actual, \
                f"expected '{platform}.{attr}' to be '{expected}', was '{actual}'"
