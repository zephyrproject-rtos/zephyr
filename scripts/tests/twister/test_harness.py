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
import re
import logging as logger

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/pylib/twister"))

from twisterlib.harness import Gtest, Bsim
from twisterlib.harness import Harness
from twisterlib.harness import Robot
from twisterlib.harness import Test
from twisterlib.testinstance import TestInstance
from twisterlib.harness import Console
from twisterlib.harness import Pytest
from twisterlib.harness import PytestHarnessException
from twisterlib.harness import HarnessImporter

GTEST_START_STATE = " RUN      "
GTEST_PASS_STATE = "       OK "
GTEST_SKIP_STATE = " DISABLED "
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


TEST_DATA_1 = [('RunID: 12345', False, False, False, None, True),
                ('PROJECT EXECUTION SUCCESSFUL', False, False, False, 'passed', False),
                ('PROJECT EXECUTION SUCCESSFUL', True, False, False, 'failed', False),
                ('PROJECT EXECUTION FAILED', False, False, False, 'failed', False),
                ('ZEPHYR FATAL ERROR', False, True, False, None, False),
                ('GCOV_COVERAGE_DUMP_START', None, None, True, None, False),
                ('GCOV_COVERAGE_DUMP_END', None, None, False, None, False),]
@pytest.mark.parametrize(
    "line, fault, fail_on_fault, cap_cov, exp_stat, exp_id",
    TEST_DATA_1,
    ids=["match id", "passed passed", "passed failed", "failed failed", "fail on fault", "GCOV START", "GCOV END"]
)
def test_harness_process_test(line, fault, fail_on_fault, cap_cov, exp_stat, exp_id):
    #Arrange
    harness = Harness()
    harness.run_id = 12345
    harness.state = None
    harness.fault = fault
    harness.fail_on_fault = fail_on_fault

    #Act
    harness.process_test(line)

    #Assert
    assert harness.matched_run_id == exp_id
    assert harness.state == exp_stat
    assert harness.capture_coverage == cap_cov


def test_robot_configure(tmp_path):
    #Arrange
    mock_platform = mock.Mock()
    mock_platform.name = "mock_platform"
    mock_platform.normalized_name = "mock_platform"

    mock_testsuite = mock.Mock(id = 'id', testcases = [])
    mock_testsuite.name = "mock_testsuite"
    mock_testsuite.harness_config = {}

    outdir = tmp_path / 'gtest_out'
    outdir.mkdir()

    instance = TestInstance(testsuite=mock_testsuite, platform=mock_platform, outdir=outdir)
    instance.testsuite.harness_config = {
        'robot_test_path': '/path/to/robot/test'
    }
    robot_harness = Robot()

    #Act
    robot_harness.configure(instance)

    #Assert
    assert robot_harness.instance == instance
    assert robot_harness.path == '/path/to/robot/test'


def test_robot_handle(tmp_path):
    #Arrange
    mock_platform = mock.Mock()
    mock_platform.name = "mock_platform"
    mock_platform.normalized_name = "mock_platform"

    mock_testsuite = mock.Mock(id = 'id', testcases = [])
    mock_testsuite.name = "mock_testsuite"
    mock_testsuite.harness_config = {}

    outdir = tmp_path / 'gtest_out'
    outdir.mkdir()

    instance = TestInstance(testsuite=mock_testsuite, platform=mock_platform, outdir=outdir)

    handler = Robot()
    handler.instance = instance
    handler.id = 'test_case_1'

    line = 'Test case passed'

    #Act
    handler.handle(line)
    tc = instance.get_case_or_create('test_case_1')

    #Assert
    assert instance.state == "passed"
    assert tc.status == "passed"


TEST_DATA_2 = [("", 0, "passed"), ("Robot test failure: sourcedir for mock_platform", 1, "failed"),]
@pytest.mark.parametrize(
    "exp_out, returncode, expected_status",
    TEST_DATA_2,
    ids=["passed", "failed"]
)
def test_robot_run_robot_test(tmp_path, caplog, exp_out, returncode, expected_status):
    # Arrange
    command = "command"

    handler = mock.Mock()
    handler.sourcedir = "sourcedir"
    handler.log = "handler.log"

    path = "path"

    mock_platform = mock.Mock()
    mock_platform.name = "mock_platform"
    mock_platform.normalized_name = "mock_platform"

    mock_testsuite = mock.Mock(id = 'id', testcases = [mock.Mock()])
    mock_testsuite.name = "mock_testsuite"
    mock_testsuite.harness_config = {}

    outdir = tmp_path / 'gtest_out'
    outdir.mkdir()

    instance = TestInstance(testsuite=mock_testsuite, platform=mock_platform, outdir=outdir)
    instance.build_dir = "build_dir"

    open_mock = mock.mock_open()

    robot = Robot()
    robot.path = path
    robot.instance = instance
    proc_mock = mock.Mock(
        returncode = returncode,
        communicate = mock.Mock(return_value=(b"output", None))
    )
    popen_mock = mock.Mock(return_value = mock.Mock(
        __enter__ = mock.Mock(return_value = proc_mock),
        __exit__ = mock.Mock()
    ))

    # Act
    with mock.patch("subprocess.Popen", popen_mock) as mock.mock_popen, \
         mock.patch("builtins.open", open_mock):
        robot.run_robot_test(command,handler)


    # Assert
    assert instance.status == expected_status
    open_mock().write.assert_called_once_with("output")
    assert exp_out in caplog.text


TEST_DATA_3 = [('one_line', None), ('multi_line', 2),]
@pytest.mark.parametrize(
    "type, num_patterns",
    TEST_DATA_3,
    ids=["one line", "multi line"]
)
def test_console_configure(tmp_path, type, num_patterns):
    #Arrange
    mock_platform = mock.Mock()
    mock_platform.name = "mock_platform"
    mock_platform.normalized_name = "mock_platform"

    mock_testsuite = mock.Mock(id = 'id', testcases = [])
    mock_testsuite.name = "mock_testsuite"
    mock_testsuite.harness_config = {}

    outdir = tmp_path / 'gtest_out'
    outdir.mkdir()

    instance = TestInstance(testsuite=mock_testsuite, platform=mock_platform, outdir=outdir)
    instance.testsuite.harness_config = {
        'type': type,
        'regex': ['pattern1', 'pattern2']
    }
    console = Console()

    #Act
    console.configure(instance)

    #Assert
    if num_patterns == 2:
        assert len(console.patterns) == num_patterns
        assert [pattern.pattern for pattern in console.patterns] == ['pattern1', 'pattern2']
    else:
        assert console.pattern.pattern == 'pattern1'


TEST_DATA_4 = [("one_line", True, "passed", "line", False, False),
                ("multi_line", True, "passed", "line", False, False),
                ("multi_line", False, "passed", "line", False, False),
                ("invalid_type", False, None, "line", False, False),
                ("invalid_type", False, None, "ERROR", True, False),
                ("invalid_type", False, None, "COVERAGE_START", False, True),
                ("invalid_type", False, None, "COVERAGE_END", False, False)]
@pytest.mark.parametrize(
    "line_type, ordered_val, exp_state, line, exp_fault, exp_capture",
    TEST_DATA_4,
    ids=["one line", "multi line ordered", "multi line not ordered", "logger error", "fail on fault", "GCOV START", "GCOV END"]
)
def test_console_handle(tmp_path, line_type, ordered_val, exp_state, line, exp_fault, exp_capture):
    mock_platform = mock.Mock()
    mock_platform.name = "mock_platform"
    mock_platform.normalized_name = "mock_platform"

    mock_testsuite = mock.Mock(id = 'id', testcases = [])
    mock_testsuite.name = "mock_testsuite"
    mock_testsuite.harness_config = {}

    outdir = tmp_path / 'gtest_out'
    outdir.mkdir()

    instance = TestInstance(testsuite=mock_testsuite, platform=mock_platform, outdir=outdir)

    console = Console()
    console.instance = instance
    console.type = line_type
    console.patterns = [re.compile("pattern1"), re.compile("pattern2")]
    console.pattern = re.compile("pattern")
    console.patterns_expected = 0
    console.state = None
    console.fail_on_fault = True
    console.FAULT = "ERROR"
    console.GCOV_START = "COVERAGE_START"
    console.GCOV_END = "COVERAGE_END"
    console.record = {"regex": "RESULT: (.*)"}
    console.fieldnames = []
    console.recording = []
    console.regex = ["regex1", "regex2"]
    console.id = "test_case_1"

    instance.get_case_or_create('test_case_1')
    instance.testsuite.id = "test_suite_1"

    console.next_pattern = 0
    console.ordered = ordered_val
    line = line
    console.handle(line)

    line1 = "pattern1"
    line2 = "pattern2"
    console.handle(line1)
    console.handle(line2)
    assert console.state == exp_state
    with pytest.raises(Exception):
        console.handle(line)
        assert logger.error.called
    assert console.fault == exp_fault
    assert console.capture_coverage == exp_capture


TEST_DATA_5 = [("serial_pty", 0), (None, 0),(None, 1)]
@pytest.mark.parametrize(
   "pty_value, hardware_value",
   TEST_DATA_5,
   ids=["hardware pty", "hardware", "non hardware"]
)

def test_pytest__generate_parameters_for_hardware(tmp_path, pty_value, hardware_value):
    #Arrange
    mock_platform = mock.Mock()
    mock_platform.name = "mock_platform"
    mock_platform.normalized_name = "mock_platform"

    mock_testsuite = mock.Mock(id = 'id', testcases = [])
    mock_testsuite.name = "mock_testsuite"
    mock_testsuite.harness_config = {}

    outdir = tmp_path / 'gtest_out'
    outdir.mkdir()

    instance = TestInstance(testsuite=mock_testsuite, platform=mock_platform, outdir=outdir)

    handler = mock.Mock()
    handler.instance = instance

    hardware = mock.Mock()
    hardware.serial_pty = pty_value
    hardware.serial = 'serial'
    hardware.baud = 115200
    hardware.runner = "runner"
    hardware.runner_params = ["--runner-param1", "runner-param2"]

    options = handler.options
    options.west_flash = "args"

    hardware.probe_id = '123'
    hardware.product = 'product'
    hardware.pre_script = 'pre_script'
    hardware.post_flash_script = 'post_flash_script'
    hardware.post_script = 'post_script'

    pytest_test = Pytest()
    pytest_test.configure(instance)

    #Act
    if hardware_value == 0:
        handler.get_hardware.return_value = hardware
        command = pytest_test._generate_parameters_for_hardware(handler)
    else:
        handler.get_hardware.return_value = None

    #Assert
    if hardware_value == 1:
        with pytest.raises(PytestHarnessException) as exinfo:
            pytest_test._generate_parameters_for_hardware(handler)
        assert str(exinfo.value) == 'Hardware is not available'
    else:
        assert '--device-type=hardware' in command
        if pty_value == "serial_pty":
            assert '--device-serial-pty=serial_pty' in command
        else:
            assert '--device-serial=serial' in command
            assert '--device-serial-baud=115200' in command
        assert '--runner=runner' in command
        assert '--runner-params=--runner-param1' in command
        assert '--runner-params=runner-param2' in command
        assert '--west-flash-extra-args=args' in command
        assert '--device-id=123' in command
        assert '--device-product=product' in command
        assert '--pre-script=pre_script' in command
        assert '--post-flash-script=post_flash_script' in command
        assert '--post-script=post_script' in command


def test__update_command_with_env_dependencies():
    cmd = ['cmd']
    pytest_test = Pytest()
    mock.patch.object(Pytest, 'PYTEST_PLUGIN_INSTALLED', False)

    # Act
    result_cmd, _ = pytest_test._update_command_with_env_dependencies(cmd)

    # Assert
    assert result_cmd == ['cmd', '-p', 'twister_harness.plugin']


def test_pytest_run(tmp_path, caplog):
    # Arrange
    timeout = 10
    cmd=['command']
    exp_out = 'Support for handler handler_type not implemented yet'

    harness = Pytest()
    harness = mock.create_autospec(harness)

    mock.patch.object(Pytest, 'generate_command', return_value=cmd)
    mock.patch.object(Pytest, 'run_command')

    mock_platform = mock.Mock()
    mock_platform.name = "mock_platform"
    mock_platform.normalized_name = "mock_platform"

    mock_testsuite = mock.Mock(id = 'id', testcases = [], source_dir = 'source_dir', harness_config = {})
    mock_testsuite.name = "mock_testsuite"
    mock_testsuite.harness_config = {}

    handler = mock.Mock(
        options = mock.Mock(verbose= 0),
        type_str = 'handler_type'
    )

    outdir = tmp_path / 'gtest_out'
    outdir.mkdir()

    instance = TestInstance(testsuite=mock_testsuite, platform=mock_platform, outdir=outdir)
    instance.handler = handler

    test_obj = Pytest()
    test_obj.configure(instance)

    # Act
    test_obj.pytest_run(timeout)
    # Assert
    assert test_obj.state == 'failed'
    assert exp_out in caplog.text


TEST_DATA_6 = [(None), ('Test')]
@pytest.mark.parametrize(
   "name",
   TEST_DATA_6,
   ids=["no name", "provided name"]
)
def test_get_harness(name):
    #Arrange
    harnessimporter = HarnessImporter()
    harness_name = name

    #Act
    harness_class = harnessimporter.get_harness(harness_name)

    #Assert
    assert isinstance(harness_class, Test)


TEST_DATA_7 = [("", "Running TESTSUITE suite_name", ['suite_name'], None, True, None),
            ("", "START - test_testcase", [], "started", True, None),
            ("", "PASS - test_example in 0 seconds", [], "passed", True, None),
            ("", "SKIP - test_example in 0 seconds", [], "skipped", True, None),
            ("", "FAIL - test_example in 0 seconds", [], "failed", True, None),
            ("not a ztest and no state for  test_id", "START - test_testcase", [], "passed", False, "passed"),
            ("not a ztest and no state for  test_id", "START - test_testcase", [], "failed", False, "failed")]
@pytest.mark.parametrize(
   "exp_out, line, exp_suite_name, exp_status, ztest, state",
   TEST_DATA_7,
   ids=['testsuite', 'testcase', 'pass', 'skip', 'failed', 'ztest pass', 'ztest fail']
)
def test_test_handle(tmp_path, caplog, exp_out, line, exp_suite_name, exp_status, ztest, state):
    # Arrange
    line = line
    mock_platform = mock.Mock()
    mock_platform.name = "mock_platform"
    mock_platform.normalized_name = "mock_platform"

    mock_testsuite = mock.Mock(id = 'id', testcases = [])
    mock_testsuite.name = "mock_testsuite"
    mock_testsuite.harness_config = {}

    outdir = tmp_path / 'gtest_out'
    outdir.mkdir()

    instance = TestInstance(testsuite=mock_testsuite, platform=mock_platform, outdir=outdir)

    test_obj = Test()
    test_obj.configure(instance)
    test_obj.id = "test_id"
    test_obj.ztest = ztest
    test_obj.state = state
    test_obj.id = 'test_id'
    #Act
    test_obj.handle(line)

    # Assert
    assert test_obj.detected_suite_names == exp_suite_name
    assert exp_out in caplog.text
    if not "Running" in line and exp_out == "":
        assert test_obj.instance.testcases[0].status == exp_status
    if "ztest" in exp_out:
        assert test_obj.instance.testcases[1].status == exp_status


@pytest.fixture
def gtest(tmp_path):
    mock_platform = mock.Mock()
    mock_platform.name = "mock_platform"
    mock_platform.normalized_name = "mock_platform"
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


def test_gtest_skipped(gtest):
    process_logs(
        gtest,
        [
            SAMPLE_GTEST_START,
            SAMPLE_GTEST_FMT.format(
                state=GTEST_START_STATE, suite="suite_name", test="test_name"
            ),
            SAMPLE_GTEST_FMT.format(
                state=GTEST_SKIP_STATE, suite="suite_name", test="test_name"
            ),
        ],
    )
    assert gtest.state is None
    assert len(gtest.detected_suite_names) == 1
    assert gtest.detected_suite_names[0] == "suite_name"
    assert gtest.instance.get_case_by_name("id.suite_name.test_name") is not None
    assert gtest.instance.get_case_by_name("id.suite_name.test_name").status == "skipped"


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


def test_gtest_one_skipped(gtest):
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
            SAMPLE_GTEST_FMT.format(
                state=GTEST_START_STATE, suite="suite_name", test="test_name1"
            ),
            SAMPLE_GTEST_FMT.format(
                state=GTEST_SKIP_STATE, suite="suite_name", test="test_name1"
            ),
            SAMPLE_GTEST_END,
        ],
    )
    assert gtest.state == "passed"
    assert len(gtest.detected_suite_names) == 1
    assert gtest.detected_suite_names[0] == "suite_name"
    assert gtest.instance.get_case_by_name("id.suite_name.test_name") is not None
    assert gtest.instance.get_case_by_name("id.suite_name.test_name").status == "passed"
    assert gtest.instance.get_case_by_name("id.suite_name.test_name1") is not None
    assert gtest.instance.get_case_by_name("id.suite_name.test_name1").status == "skipped"


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
