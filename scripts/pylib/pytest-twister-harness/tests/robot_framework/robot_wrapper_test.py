import pathlib
import sys
from unittest.mock import patch

import pytest
from twister_harness.robot_framework import robot_wrapper


def test__integration__dry_run_robotfile__no_errors():
    robot_file = pathlib.Path(__file__).parent / "device_test.robot"
    cli_arguments = [
        "twister_harness.robot_framework.robot_wrapper",
        "--quiet",
        "--dryrun",
        "--build-dir",
        ".",
        "--device-type=custom",
        str(robot_file),
    ]
    with patch.object(sys, "argv", cli_arguments):
        with pytest.raises(SystemExit) as excinfo:
            robot_wrapper.main()

        assert excinfo.value.code == 0


def test__integration__missing_parameter__raises_nonzero_systemexit():
    robot_file = pathlib.Path(__file__).parent / "device_test.robot"
    cli_arguments = [
        "twister_harness.robot_framework.robot_wrapper",
        "--quiet",
        "--dryrun",
        "--build-dir",
        ".",
        "--device-type=custom",
    ]
    with patch.object(sys, "argv", cli_arguments):
        with pytest.raises(SystemExit) as excinfo:
            robot_wrapper.main()

        assert excinfo.value.code != 0
