# Copyright (c) 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0
from __future__ import annotations

import textwrap
from pathlib import Path

import pytest
import robot
from twisterlib.harness import Robotframework
from twisterlib.testinstance import TestInstance


@pytest.mark.parametrize("device_type", ["native", "qemu"])
def test_robot_command(testinstance: TestInstance, device_type):
    robot_harness = Robotframework()
    robot_harness.configure(testinstance)

    testinstance.handler.type_str = device_type
    ref_command = [
        "twister_harness.robot_framework.robot_wrapper",
        "--log",
        f"{testinstance.build_dir}/robot.html",
        "--xunit",
        f"{testinstance.build_dir}/robot_report.xml",
        f"--outputdir={testinstance.build_dir}/robot",
        f"--build-dir={testinstance.build_dir}",
    ]

    command = robot_harness.generate_command()
    for c in ref_command:
        assert c in command


def test_robot_command_extra_args(testinstance: TestInstance):
    robot_harness = Robotframework()
    robot_args = ["-k test1", "-m mark1"]
    testinstance.testsuite.harness_config["robot_args"] = robot_args
    robot_harness.configure(testinstance)
    command = robot_harness.generate_command()
    for c in robot_args:
        assert c in command


def test_robot_command_extra_args_in_options(testinstance: TestInstance):
    robot_harness = Robotframework()
    robot_args_from_yaml = "-k test_from_yaml"
    robot_args_from_cmd = ["-k", "test_from_cmd"]
    testinstance.testsuite.harness_config["robot_args"] = [robot_args_from_yaml]
    testinstance.handler.options.robot_args = robot_args_from_cmd
    robot_harness.configure(testinstance)
    command = robot_harness.generate_command()
    assert robot_args_from_cmd[0] in command
    assert robot_args_from_cmd[1] in command
    assert robot_args_from_yaml not in command


@pytest.mark.parametrize(
    ("robot_test_path", "expected"),
    [
        (
            "robot/test_shell.py",
            "samples/hello/robot/test_shell.py",
        ),
        (
            "../shell/robot/test_shell.py",
            "samples/shell/robot/test_shell.py",
        ),
        (
            "/tmp/test_temp.py",
            "/tmp/test_temp.py",
        ),
        (
            "~/tmp/test_temp.py",
            "/home/joe/tmp/test_temp.py",
        ),
        (
            "$ZEPHYR_BASE/samples/subsys/testsuite/robot/shell/robot",
            "/zephyr_base/samples/subsys/testsuite/robot/shell/robot",
        ),
    ],
    ids=[
        "one_file",
        "relative_path",
        "absolute_path",
        "user_dir",
        "with_env_var",
    ],
)
def test_robot_handle_source_list(
    testinstance: TestInstance, monkeypatch, robot_test_path, expected
):
    monkeypatch.setenv("ZEPHYR_BASE", "/zephyr_base")
    monkeypatch.setenv("HOME", "/home/joe")
    testinstance.testsuite.harness_config["robot_test_path"] = robot_test_path
    robot_harness = Robotframework()
    robot_harness.configure(testinstance)
    command = robot_harness.generate_command()

    assert expected in command


def _execute_robot_test_helper(
    tmp_path, testinstance, name, test_file_content: str, **robot_args
) -> Robotframework:
    test_file = tmp_path / f"test_{name}.robot"
    test_file.write_text(test_file_content)
    report_file = Path("robot_report.xml")

    robot.run(test_file, xunit=report_file, **robot_args)

    assert report_file.is_file()

    robot_harness = Robotframework()
    robot_harness.configure(testinstance)
    robot_harness.report_file = report_file

    robot_harness._update_test_status()

    return robot_harness


def test_if_report_is_parsed(tmp_path, testinstance: TestInstance):
    test_file_content = textwrap.dedent(
        """
        *** Test Cases ***
        Test A
        \tPass Execution \t "My Message"

        Test B
        \tLog \t "My message"
    """
    )

    robot_harness = _execute_robot_test_helper(
        tmp_path, testinstance, "valid", test_file_content
    )

    assert robot_harness.state == "passed"
    assert testinstance.status == "passed"
    assert len(testinstance.testcases) == 2
    for tc in testinstance.testcases:
        assert tc.status == "passed"


def test_if_report_with_error(tmp_path, testinstance: TestInstance):
    test_file_content = textwrap.dedent(
        """
        *** Test Cases ***
        Test Error
        \tShould Be Equal \t 1 \t 42

        Test Fail
        \tFail \t "My message"
    """
    )

    robot_harness = _execute_robot_test_helper(
        tmp_path, testinstance, "error", test_file_content
    )

    assert robot_harness.state == "failed"
    assert testinstance.status == "failed"
    assert len(testinstance.testcases) == 2
    for tc in testinstance.testcases:
        assert tc.status == "failed"
        assert tc.reason
    assert testinstance.reason
    assert "2/2" in testinstance.reason


def test_if_report_with_skip(tmp_path, testinstance: TestInstance):
    test_file_content = textwrap.dedent(
        """
        *** Test Cases ***
        Test Skip
        \tSkip \t "I skip!"

        Test Skip Conditional
        \tSkip If \t True \t msg="I skip!"
    """
    )

    robot_harness = _execute_robot_test_helper(
        tmp_path, testinstance, "skip", test_file_content
    )

    assert robot_harness.state == "skipped"
    assert testinstance.status == "skipped"
    assert len(testinstance.testcases) == 2
    for tc in testinstance.testcases:
        assert tc.status == "skipped"
        assert tc.reason


def test_if_report_with_filter(tmp_path, testinstance: TestInstance):
    test_file_content = textwrap.dedent(
        """
        *** Test Cases ***
        Test A
        \tPass Execution \t "My Message"

        Test B
        \tLog \t "My message"
    """
    )

    robot_harness = _execute_robot_test_helper(
        tmp_path, testinstance, "filter", test_file_content, test="Test B"
    )

    assert robot_harness.state == "passed"
    assert testinstance.status == "passed"
    assert len(testinstance.testcases) == 1


def test_if_report_with_no_collected(tmp_path, testinstance: TestInstance):
    test_file_content = textwrap.dedent(
        """
        *** Test Cases ***
        Test A
        \tPass Execution \t "My Message"
    """
    )

    robot_harness = _execute_robot_test_helper(
        tmp_path,
        testinstance,
        "filter_skipped",
        test_file_content,
        test="Test C",
        runemptysuite=True,
    )

    assert len(testinstance.testcases) == 0
    assert robot_harness.state == "skipped"
    assert testinstance.status == "skipped"
