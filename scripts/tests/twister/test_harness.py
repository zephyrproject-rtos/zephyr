#!/usr/bin/env python3

# Copyright(c) 2023 Google LLC
# SPDX-License-Identifier: Apache-2.0

"""
This test file contains testsuites for the Harness classes of twister
"""
import mock
import sys
import os
import pytest

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/pylib/twister"))

from twisterlib.harness import Gtest, Bsim
from twisterlib.testinstance import TestInstance

GTEST_START_STATE = " RUN      "
GTEST_PASS_STATE = "       OK "
GTEST_FAIL_STATE = "  FAILED  "
SAMPLE_GTEST_START = (
    "[00:00:00.000,000] [0m<inf> label:  [==========] Running all tests.[0m"
)
SAMPLE_GTEST_FMT = "[00:00:00.000,000] [0m<inf> label:  [{state}] {suite}.{test}[0m"
SAMPLE_GTEST_END = (
    "[00:00:00.000,000] [0m<inf> label:  [==========] Done running all tests.[0m"
)


def process_logs(harness, logs):
    for line in logs:
        harness.handle(line)


@pytest.fixture
def gtest(tmp_path):
    mock_platform = mock.Mock()
    mock_platform.name = "mock_platform"
    mock_testsuite = mock.Mock()
    mock_testsuite.name = "mock_testsuite"
    mock_testsuite.detailed_test_id = True
    mock_testsuite.id = "id"
    mock_testsuite.testcases = []
    mock_testsuite.harness_config = {}
    outdir = tmp_path / 'gtest_out'
    outdir.mkdir()

    instance = TestInstance(testsuite=mock_testsuite, platform=mock_platform, outdir=outdir)

    harness = Gtest()
    harness.configure(instance)
    return harness


def test_gtest_start_test_no_suites_detected(gtest):
    process_logs(gtest, [SAMPLE_GTEST_START])
    assert len(gtest.detected_suite_names) == 0
    assert gtest.state is None


def test_gtest_start_test(gtest):
    process_logs(
        gtest,
        [
            SAMPLE_GTEST_START,
            SAMPLE_GTEST_FMT.format(
                state=GTEST_START_STATE, suite="suite_name", test="test_name"
            ),
        ],
    )
    assert gtest.state is None
    assert len(gtest.detected_suite_names) == 1
    assert gtest.detected_suite_names[0] == "suite_name"
    assert gtest.instance.get_case_by_name("id.suite_name.test_name") is not None
    assert (
        gtest.instance.get_case_by_name("id.suite_name.test_name").status == "started"
    )


def test_gtest_pass(gtest):
    process_logs(
        gtest,
        [
            SAMPLE_GTEST_START,
            SAMPLE_GTEST_FMT.format(
                state=GTEST_START_STATE, suite="suite_name", test="test_name"
            ),
            SAMPLE_GTEST_FMT.format(
                state=GTEST_PASS_STATE, suite="suite_name", test="test_name"
            ),
        ],
    )
    assert gtest.state is None
    assert len(gtest.detected_suite_names) == 1
    assert gtest.detected_suite_names[0] == "suite_name"
    assert gtest.instance.get_case_by_name("id.suite_name.test_name") is not None
    assert gtest.instance.get_case_by_name("id.suite_name.test_name").status == "passed"


def test_gtest_failed(gtest):
    process_logs(
        gtest,
        [
            SAMPLE_GTEST_START,
            SAMPLE_GTEST_FMT.format(
                state=GTEST_START_STATE, suite="suite_name", test="test_name"
            ),
            SAMPLE_GTEST_FMT.format(
                state=GTEST_FAIL_STATE, suite="suite_name", test="test_name"
            ),
        ],
    )
    assert gtest.state is None
    assert len(gtest.detected_suite_names) == 1
    assert gtest.detected_suite_names[0] == "suite_name"
    assert gtest.instance.get_case_by_name("id.suite_name.test_name") is not None
    assert gtest.instance.get_case_by_name("id.suite_name.test_name").status == "failed"


def test_gtest_all_pass(gtest):
    process_logs(
        gtest,
        [
            SAMPLE_GTEST_START,
            SAMPLE_GTEST_FMT.format(
                state=GTEST_START_STATE, suite="suite_name", test="test_name"
            ),
            SAMPLE_GTEST_FMT.format(
                state=GTEST_PASS_STATE, suite="suite_name", test="test_name"
            ),
            SAMPLE_GTEST_END,
        ],
    )
    assert gtest.state == "passed"
    assert len(gtest.detected_suite_names) == 1
    assert gtest.detected_suite_names[0] == "suite_name"
    assert gtest.instance.get_case_by_name("id.suite_name.test_name") is not None
    assert gtest.instance.get_case_by_name("id.suite_name.test_name").status == "passed"


def test_gtest_one_fail(gtest):
    process_logs(
        gtest,
        [
            SAMPLE_GTEST_START,
            SAMPLE_GTEST_FMT.format(
                state=GTEST_START_STATE, suite="suite_name", test="test0"
            ),
            SAMPLE_GTEST_FMT.format(
                state=GTEST_PASS_STATE, suite="suite_name", test="test0"
            ),
            SAMPLE_GTEST_FMT.format(
                state=GTEST_START_STATE, suite="suite_name", test="test1"
            ),
            SAMPLE_GTEST_FMT.format(
                state=GTEST_FAIL_STATE, suite="suite_name", test="test1"
            ),
            SAMPLE_GTEST_END,
        ],
    )
    assert gtest.state == "failed"
    assert len(gtest.detected_suite_names) == 1
    assert gtest.detected_suite_names[0] == "suite_name"
    assert gtest.instance.get_case_by_name("id.suite_name.test0") is not None
    assert gtest.instance.get_case_by_name("id.suite_name.test0").status == "passed"
    assert gtest.instance.get_case_by_name("id.suite_name.test1") is not None
    assert gtest.instance.get_case_by_name("id.suite_name.test1").status == "failed"


def test_gtest_missing_result(gtest):
    with pytest.raises(
        AssertionError,
        match=r"gTest error, id.suite_name.test0 didn't finish",
    ):
        process_logs(
            gtest,
            [
                SAMPLE_GTEST_START,
                SAMPLE_GTEST_FMT.format(
                    state=GTEST_START_STATE, suite="suite_name", test="test0"
                ),
                SAMPLE_GTEST_FMT.format(
                    state=GTEST_START_STATE, suite="suite_name", test="test1"
                ),
            ],
        )


def test_gtest_mismatch_result(gtest):
    with pytest.raises(
        AssertionError,
        match=r"gTest error, mismatched tests. Expected id.suite_name.test0 but got None",
    ):
        process_logs(
            gtest,
            [
                SAMPLE_GTEST_START,
                SAMPLE_GTEST_FMT.format(
                    state=GTEST_START_STATE, suite="suite_name", test="test0"
                ),
                SAMPLE_GTEST_FMT.format(
                    state=GTEST_PASS_STATE, suite="suite_name", test="test1"
                ),
            ],
        )


def test_gtest_repeated_result(gtest):
    with pytest.raises(
        AssertionError,
        match=r"gTest error, mismatched tests. Expected id.suite_name.test1 but got id.suite_name.test0",
    ):
        process_logs(
            gtest,
            [
                SAMPLE_GTEST_START,
                SAMPLE_GTEST_FMT.format(
                    state=GTEST_START_STATE, suite="suite_name", test="test0"
                ),
                SAMPLE_GTEST_FMT.format(
                    state=GTEST_PASS_STATE, suite="suite_name", test="test0"
                ),
                SAMPLE_GTEST_FMT.format(
                    state=GTEST_START_STATE, suite="suite_name", test="test1"
                ),
                SAMPLE_GTEST_FMT.format(
                    state=GTEST_PASS_STATE, suite="suite_name", test="test0"
                ),
            ],
        )


def test_gtest_repeated_run(gtest):
    with pytest.raises(
        AssertionError,
        match=r"gTest error, id.suite_name.test0 running twice",
    ):
        process_logs(
            gtest,
            [
                SAMPLE_GTEST_START,
                SAMPLE_GTEST_FMT.format(
                    state=GTEST_START_STATE, suite="suite_name", test="test0"
                ),
                SAMPLE_GTEST_FMT.format(
                    state=GTEST_PASS_STATE, suite="suite_name", test="test0"
                ),
                SAMPLE_GTEST_FMT.format(
                    state=GTEST_START_STATE, suite="suite_name", test="test0"
                ),
            ],
        )


def test_bsim_build(monkeypatch, tmp_path):
    mocked_instance = mock.Mock()
    build_dir = tmp_path / 'build_dir'
    os.makedirs(build_dir)
    mocked_instance.build_dir = str(build_dir)
    mocked_instance.name = 'platform_name/test/dummy.test'
    mocked_instance.testsuite.harness_config = {}

    harness = Bsim()
    harness.instance = mocked_instance

    monkeypatch.setenv('BSIM_OUT_PATH', str(tmp_path))
    os.makedirs(os.path.join(tmp_path, 'bin'), exist_ok=True)
    zephyr_exe_path = os.path.join(build_dir, 'zephyr', 'zephyr.exe')
    os.makedirs(os.path.dirname(zephyr_exe_path), exist_ok=True)
    with open(zephyr_exe_path, 'w') as file:
        file.write('TEST_EXE')

    harness.build()

    new_exe_path = os.path.join(tmp_path, 'bin', 'bs_platform_name_test_dummy_test')
    assert os.path.exists(new_exe_path)
    with open(new_exe_path, 'r') as file:
        exe_content = file.read()
    assert 'TEST_EXE' in exe_content
