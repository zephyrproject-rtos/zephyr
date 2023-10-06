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

from twisterlib.harness import Gtest
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

RUN_PASSED = "PROJECT EXECUTION SUCCESSFUL"
RUN_FAILED = "PROJECT EXECUTION FAILED"
test_suite_start_pattern = r"Running TESTSUITE (?P<suite_name>.*)"
ZTEST_START_PATTERN = r"START - (test_)?([a-zA-Z0-9_-]+)"

def handle(self, line):
    test_suite_match = re.search(self.test_suite_start_pattern, line)
    if test_suite_match:
        suite_name = test_suite_match.group("suite_name")
        self.detected_suite_names.append(suite_name)

    testcase_match = re.search(self.ZTEST_START_PATTERN, line)
    if testcase_match:
        name = "{}.{}".format(self.id, testcase_match.group(2))
        tc = self.instance.get_case_or_create(name)
        # Mark the test as started, if something happens here, it is mostly
        # due to this tests, for example timeout. This should in this case
        # be marked as failed and not blocked (not run).
        tc.status = "started"

    if testcase_match or self._match:
        self.testcase_output += line + "\n"
        self._match = True

    result_match = result_re.match(line)

    if result_match:
        matched_status = result_match.group(1)
        name = "{}.{}".format(self.id, result_match.group(3))
        tc = self.instance.get_case_or_create(name)
        tc.status = self.ztest_to_status[matched_status]
        if tc.status == "skipped":
            tc.reason = "ztest skip"
        tc.duration = float(result_match.group(4))
        if tc.status == "failed":
            tc.output = self.testcase_output
        self.testcase_output = ""
        self._match = False
        self.ztest = True

    self.process_test(line)

    if not self.ztest and self.state:
        logger.debug(f"not a ztest and no state for  {self.id}")
        tc = self.instance.get_case_or_create(self.id)
        if self.state == "passed":
            tc.status = "passed"
        else:
            tc.status = "failed"

# fail_on_fault is True and line contains FAULT, sets fault to True
def test_fail_on_fault_is_true_and_line_contains_fault_sets_fault_to_true():
    harness = Harness()
    harness.fail_on_fault = True
    harness.fault = False
    line = "ZEPHYR FATAL ERROR"
    harness.process_test(line)
    assert harness.fault == True

# line contains GCOV_START: capture_coverage set to True
def test_line_contains_gcov_start_capture_coverage_set_to_true():
    harness = Harness()
    line = "GCOV_COVERAGE_DUMP_START"
    harness.process_test(line)
    assert harness.capture_coverage == True

# line contains GCOV_END: capture_coverage set to False
def test_line_contains_gcov_end_capture_coverage_set_to_false():
    harness = Harness()
    line = "GCOV_COVERAGE_DUMP_END"
    harness.process_test(line)
    assert harness.capture_coverage == False

# instance has a testsuite and a harness_config
def test_instance_has_testsuite_and_harness_config():
    mock_platform = mock.Mock()
    mock_platform.name = "mock_platform"
    mock_testsuite = mock.Mock()
    mock_testsuite.name = "mock_testsuite"
    mock_testsuite.id = "id"
    mock_testsuite.testcases = []
    instance = TestInstance(testsuite=mock_testsuite, platform=mock_platform, outdir="")
    instance.testsuite.harness_config = {
        'robot_test_path': '/path/to/robot/test'
    }
    robot_harness = Robot()
    robot_harness.configure(instance)
    assert robot_harness.instance == instance
    assert robot_harness.path == '/path/to/robot/test'

# sets instance state to "passed"
def test_sets_instance_state_to_passed():
    mock_platform = mock.Mock()
    mock_platform.name = "mock_platform"
    mock_testsuite = mock.Mock()
    mock_testsuite.name = "mock_testsuite"
    mock_testsuite.id = "id"
    mock_testsuite.testcases = []
    instance = TestInstance(testsuite=mock_testsuite, platform=mock_platform, outdir="")
    handler = Robot()
    handler.instance = instance
    handler.id = 'test_case_1'
    line = 'Test case passed'
    handler.handle(line)
    assert instance.state == "passed"

# sets test case status to "passed"
def test_sets_test_case_status_to_passed():
    mock_platform = mock.Mock()
    mock_platform.name = "mock_platform"
    mock_testsuite = mock.Mock()
    mock_testsuite.name = "mock_testsuite"
    mock_testsuite.id = "id"
    mock_testsuite.testcases = []
    instance = TestInstance(testsuite=mock_testsuite, platform=mock_platform, outdir="")
    handler = Robot()
    handler.instance = instance
    handler.id = 'test_case_1'
    line = 'Test case passed'
    handler.handle(line)
    tc = instance.get_case_or_create('test_case_1')
    assert tc.status == "passed"

# Test is executed successfully and returns 0
def test_successful_execution():
    # Mock the subprocess.Popen class
    mock_popen = mock.patch('subprocess.Popen')
    # Set the returncode of the mock process to 0
    mock_popen.return_value = mock.Mock(returncode=0)

    # Create a mock instance and set the necessary attributes
    instance = mock.Mock()
    instance.build_dir = '/path/to/build/dir'
    instance.execution_time = 0

    # Create a mock handler and set the necessary attributes
    handler = mock.Mock()
    handler.sourcedir = '/path/to/source/dir'
    handler.log = 'test.log'

    # Create a Robot object and call the run_robot_test method
    robot = Robot()
    robot.instance = instance
    robot.path = '/path/to/robot/files'
    robot.run_robot_test(['robot', 'test.robot'], handler)

    # Assert that the subprocess.Popen class was called with the correct arguments
    mock_popen.assert_called_once_with(['robot', 'test.robot'], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, cwd='/path/to/build/dir', env={'ROBOT_FILES': '/path/to/robot/files'})

    # Assert that the instance status is set to "passed"
    assert robot.instance.status == "passed"
    
@pytest.fixture
def gtest():
    mock_platform = mock.Mock()
    mock_platform.name = "mock_platform"
    mock_testsuite = mock.Mock()
    mock_testsuite.name = "mock_testsuite"
    mock_testsuite.detailed_test_id = True
    mock_testsuite.id = "id"
    mock_testsuite.testcases = []
    instance = TestInstance(testsuite=mock_testsuite, platform=mock_platform, outdir="")

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
