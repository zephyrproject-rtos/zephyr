#!/usr/bin/env python3
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
"""

import os
import sys
import pytest

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/sanity_chk"))

from sanitylib import TestCase, TestInstance, Platform, TestSuite


testdata = [
    ("/testcase/root", ".", "a.b.c", "a.b.c"),
    (ZEPHYR_BASE, "sample/x/y", "a.b.c", "sample/x/y/a.b.c"),
    (ZEPHYR_BASE, ".", "a.b.c", "a.b.c"),
    (os.path.join(ZEPHYR_BASE, "foo"), ".", "a.b.c", "foo/a.b.c"),
    (os.path.join(ZEPHYR_BASE, "foo"), ".", "", "foo"),
]

@pytest.mark.parametrize("root,workdir,name,expected", testdata)
def test_testcase_name_001(root, workdir, name, expected):
    tc = TestCase(root, workdir, name)
    assert tc.name == expected


def test_testcase_subcases_001(test_data):
    tc = TestCase("/testcase/root", ".", "a.b.c")

    results, warnings = tc.scan_file(os.path.join(test_data, "test_subtest.c"))
    expected_subcases = ['a', 'b', 'c', 'unit_a', 'unit_b', 'newline', 'aa', 'user', 'last']
    assert results == expected_subcases
    assert warnings == None

    subcases = tc.scan_path(test_data)
    assert subcases == expected_subcases

    tc.id = "test_id"
    tc.parse_subcases(test_data)

    assert tc.cases == [tc.id + "." + x for x in expected_subcases]


testdata1 = [
    (False, False, "console", "na", (False, False, False, []), (True, False)),
    (False, False, "console", "native", (False, False, False, []), (False, True)),
    (False, False, "console", "native", (True, False, False, []), (True, False)),
    (False, False, "console", "native", (True, True, False, []), (True, False)),
    (True, False, "console", "native", (True, True, False, []), (True, False)),
    (False, False, "sensor", "native", (True, True, False, []), (True, False)),
]

@pytest.mark.parametrize("build_only,slow,harness,plat_type,param,expected", testdata1)
def test_testcase_check_build_or_run_001(build_only, slow, harness, plat_type,param, expected):
    tc = TestCase(os.path.join(ZEPHYR_BASE, "foo"), ".", "a.b.c")
    tc.id = "test_id"

    plat = Platform()
    plat.name = "platform"
    plat.type = plat_type

    ti = TestInstance(tc, plat, "")

    ti.check_build_or_run()
    assert ti.build_only and not ti.run

    tc.build_only = build_only
    tc.slow = slow
    tc.harness = harness

    ti = TestInstance(tc, plat, "")

    # build_only=False, enable_slow=False, device_testing=False, fixture=[]):
    bo, sl, dev, fix = param
    ti.check_build_or_run(bo, sl, dev, fix)
    b, r = expected
    assert ti.build_only == b
    assert ti.run == r


def test_testcase_003(test_data, monkeypatch):

    monkeypatch.setenv("ZEPHYR_TOOLCHAIN_VARIANT", "zephyr")
    # board_root_list, testcase_roots, outdir)
    ts = TestSuite(outdir="foo")

    plat = Platform()
    plat.name = "platform"
    plat.supported_toolchains = ['zephyr']

    ts.platforms.append(plat)

    tc = TestCase(os.path.join(ZEPHYR_BASE, "foo"), ".", "a.b.c")
    tc.id = "test_id"
    tc.parse_subcases(test_data)
    ts.testcases[tc.name] = tc

    expected_subcases = ['a', 'b', 'c', 'unit_a', 'unit_b', 'newline', 'aa', 'user', 'last']
    all = ts.get_all_tests()
    assert all == [tc.id + "." + x for x in expected_subcases]

    assert ts.get_platform("platform").name == plat.name

    discards = ts.apply_filters()
    ti = next(iter(discards))
    assert discards.get(ti) == "Not a default test platform"

    plat.default = True
    discards = ts.apply_filters()
    assert not discards

    plat.ram = 10
    tc.min_ram = 20
    discards = ts.apply_filters()
    ti = next(iter(discards))
    assert discards.get(ti) == "Not enough RAM"

    tc.min_ram = -1
    plat.flash = 10
    tc.min_flash = 20
    discards = ts.apply_filters()
    ti = next(iter(discards))
    assert discards.get(ti) == "Not enough FLASH"

    tc.min_ram = -1
    plat.flash = 20
    tc.min_flash = 20
    discards = ts.apply_filters()
    assert not discards





