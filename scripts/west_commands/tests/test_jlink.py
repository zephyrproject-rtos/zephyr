# Copyright (c) 2024 Qorvo US, Inc.
# SPDX-License-Identifier: Apache-2.0

import argparse
import platform
import tempfile
from pathlib import Path
from unittest.mock import call, patch

from runners.core import FileType
from runners.jlink import JLinkBinaryRunner

WEST_CMDS_DIR = Path(__file__).parent.parent.absolute()

TEST_JLINK_CLI = "JLink.exe" if platform.system() == "Windows" else "JLinkExe"
TEST_JLINK_DEV = "jlink-test-dev"
TEST_JLINK_ID = "12345678"
TEST_JLINK_SPEED = "2000000"
TEST_FLASH_SCRIPT = "test_flash_script"
TEST_TMP_DIR_INIT = tempfile.TemporaryDirectory(suffix="jlink")
TEST_TMP_DIR_CREAT = tempfile.TemporaryDirectory(suffix="jlink")


def require_patch(program):
    assert program in [TEST_JLINK_CLI, "/opt/SEGGER/JLink_V798e/JLinkExe"]
    return program


def parse_patch():
    pass


def tempdir_init_patch(suffix=None, prefix=None, dir=None, ignore_cleanup_errors=None):
    return TEST_TMP_DIR_INIT


def tempdir_creat_patch(suffix=None, prefix=None, dir=None, ignore_cleanup_errors=None):
    return TEST_TMP_DIR_CREAT


def pylink_library_patch(path):
    breakpoint()
    return path


@patch("runners.core.BuildConfiguration._parse", side_effect=parse_patch)
@patch("runners.core.ZephyrBinaryRunner.require", side_effect=require_patch)
@patch("tempfile.TemporaryDirectory", side_effect=tempdir_init_patch)
@patch("pylink.library.__init__", side_effect=pylink_library_patch)
@patch("runners.core.ZephyrBinaryRunner.check_call")
def test_jlink_init(cc, jlinklib, tdir, req, runner_config):
    runner = JLinkBinaryRunner(runner_config, TEST_JLINK_DEV, commander="JLinkExe")
    runner.file_type = FileType.HEX
    runner._jlink_version = (6, 80, 0)
    runner.run("flash")
    EXPECTED_COMMANDS = [
        [
            f"{WEST_CMDS_DIR}/JLinkExe",
            "-nogui",
            "1",
            "-if",
            "swd",
            "-speed",
            "auto",
            "-device",
            TEST_JLINK_DEV,
            "-CommanderScript",
            f"{TEST_TMP_DIR_INIT.name}/runner.jlink",
            "-nogui",
            "1",
        ]
    ]
    # cc.call_args_list has stdout=-3...
    exp = call(cc.call_args_list[0][0])
    assert exp == [call(x) for x in EXPECTED_COMMANDS]


@patch("runners.core.BuildConfiguration._parse", side_effect=parse_patch)
@patch("runners.core.ZephyrBinaryRunner.require", side_effect=require_patch)
@patch("tempfile.TemporaryDirectory", side_effect=tempdir_creat_patch)
@patch("runners.core.ZephyrBinaryRunner.check_call")
def test_jlink_create(cc, tdir, req, runner_config):
    args = [
        "--commander",
        TEST_JLINK_CLI,
        "--id",
        TEST_JLINK_ID,
        "--device",
        TEST_JLINK_DEV,
        "--speed",
        TEST_JLINK_SPEED,
    ]
    parser = argparse.ArgumentParser(allow_abbrev=False)
    JLinkBinaryRunner.add_parser(parser)
    arg_namespace = parser.parse_args(args)
    runner = JLinkBinaryRunner.create(runner_config, arg_namespace)
    runner.file_type = FileType.HEX
    runner._jlink_version = (6, 80, 0)
    runner.run("flash")
    EXPECTED_COMMANDS = [
        f"{WEST_CMDS_DIR}/{TEST_JLINK_CLI}",
        "-USB",
        TEST_JLINK_ID,
        "-nogui",
        "1",
        "-if",
        "swd",
        "-speed",
        TEST_JLINK_SPEED,
        "-device",
        TEST_JLINK_DEV,
        "-CommanderScript",
        f"{TEST_TMP_DIR_CREAT.name}/runner.jlink",
        "-nogui",
        "1",
    ]
    assert cc.call_args_list[0].args[0] == EXPECTED_COMMANDS


@patch("runners.core.BuildConfiguration._parse", side_effect=parse_patch)
@patch("runners.core.ZephyrBinaryRunner.require", side_effect=require_patch)
@patch("tempfile.TemporaryDirectory", side_effect=tempdir_creat_patch)
@patch("runners.core.ZephyrBinaryRunner.check_call")
def test_jlink_create_with_cmdscript(cc, tdir, req, runner_config):
    args = [
        "--commander",
        TEST_JLINK_CLI,
        "--id",
        TEST_JLINK_ID,
        "--device",
        TEST_JLINK_DEV,
        "--speed",
        TEST_JLINK_SPEED,
        "--flash-script",
        TEST_FLASH_SCRIPT,
    ]
    parser = argparse.ArgumentParser(allow_abbrev=False)
    JLinkBinaryRunner.add_parser(parser)
    arg_namespace = parser.parse_args(args)
    runner = JLinkBinaryRunner.create(runner_config, arg_namespace)
    runner.file_type = FileType.HEX
    runner._jlink_version = (6, 80, 0)
    runner.run("flash")
    EXPECTED_COMMANDS = [
        f"{WEST_CMDS_DIR}/{TEST_JLINK_CLI}",
        "-USB",
        TEST_JLINK_ID,
        "-nogui",
        "1",
        "-if",
        "swd",
        "-speed",
        TEST_JLINK_SPEED,
        "-device",
        TEST_JLINK_DEV,
        "-CommanderScript",
        TEST_FLASH_SCRIPT,
        "-nogui",
        "1",
    ]
    assert cc.call_args_list[0].args[0] == EXPECTED_COMMANDS
