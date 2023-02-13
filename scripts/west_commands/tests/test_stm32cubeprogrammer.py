# Copyright (c) 2020 Teslabs Engineering S.L.
#
# SPDX-License-Identifier: Apache-2.0

import argparse
from pathlib import Path
from unittest.mock import patch, call

import pytest

from runners.stm32cubeprogrammer import STM32CubeProgrammerBinaryRunner
from conftest import RC_KERNEL_HEX, RC_KERNEL_ELF


CLI_PATH = Path("STM32_Programmer_CLI")
"""Default CLI path used in tests."""

HOME_PATH = Path("/home", "test")
"""Home path (used for Linux system CLI path)."""

PROGRAMFILESX86_PATH = Path("C:", "Program Files (x86)")
"""Program files x86 path (used for Windows system CLI path)."""

ENVIRON = {
    "PROGRAMFILES(X86)": str(PROGRAMFILESX86_PATH),
}
"""Environment (used for Windows system CLI path)."""

LINUX_CLI_PATH = (
    HOME_PATH
    / "STMicroelectronics"
    / "STM32Cube"
    / "STM32CubeProgrammer"
    / "bin"
    / "STM32_Programmer_CLI"
)
"""Linux CLI path."""

WINDOWS_CLI_PATH = (
    PROGRAMFILESX86_PATH
    / "STMicroelectronics"
    / "STM32Cube"
    / "STM32CubeProgrammer"
    / "bin"
    / "STM32_Programmer_CLI.exe"
)
"""Windows CLI path."""

MACOS_CLI_PATH = (
    Path("/Applications")
    / "STMicroelectronics"
    / "STM32Cube"
    / "STM32CubeProgrammer"
    / "STM32CubeProgrammer.app"
    / "Contents"
    / "MacOs"
    / "bin"
    / "STM32_Programmer_CLI"
)
"""macOS CLI path."""

TEST_CASES = (
    {
        "port": "swd",
        "frequency": None,
        "reset_mode": None,
        "conn_modifiers": None,
        "cli": CLI_PATH,
        "use_elf": False,
        "erase": False,
        "tool_opt": [],
        "system": "",
        "cli_path": str(CLI_PATH),
        "calls": [
            [
                str(CLI_PATH),
                "--connect",
                "port=swd",
                "--download",
                RC_KERNEL_HEX,
                "--start",
            ],
        ],
    },
    {
        "port": "swd",
        "frequency": "4000",
        "reset_mode": None,
        "conn_modifiers": None,
        "cli": CLI_PATH,
        "use_elf": False,
        "erase": False,
        "tool_opt": [],
        "system": "",
        "cli_path": str(CLI_PATH),
        "calls": [
            [
                str(CLI_PATH),
                "--connect",
                "port=swd freq=4000",
                "--download",
                RC_KERNEL_HEX,
                "--start",
            ],
        ],
    },
    {
        "port": "swd",
        "frequency": None,
        "reset_mode": "hw",
        "conn_modifiers": None,
        "cli": CLI_PATH,
        "use_elf": False,
        "erase": False,
        "tool_opt": [],
        "system": "",
        "cli_path": str(CLI_PATH),
        "calls": [
            [
                str(CLI_PATH),
                "--connect",
                "port=swd reset=HWrst",
                "--download",
                RC_KERNEL_HEX,
                "--start",
            ],
        ],
    },
    {
        "port": "swd",
        "frequency": None,
        "reset_mode": "sw",
        "conn_modifiers": None,
        "cli": CLI_PATH,
        "use_elf": False,
        "erase": False,
        "tool_opt": [],
        "system": "",
        "cli_path": str(CLI_PATH),
        "calls": [
            [
                str(CLI_PATH),
                "--connect",
                "port=swd reset=SWrst",
                "--download",
                RC_KERNEL_HEX,
                "--start",
            ],
        ],
    },
    {
        "port": "swd",
        "frequency": None,
        "reset_mode": "core",
        "conn_modifiers": None,
        "cli": CLI_PATH,
        "use_elf": False,
        "erase": False,
        "tool_opt": [],
        "system": "",
        "cli_path": str(CLI_PATH),
        "calls": [
            [
                str(CLI_PATH),
                "--connect",
                "port=swd reset=Crst",
                "--download",
                RC_KERNEL_HEX,
                "--start",
            ],
        ],
    },
    {
        "port": "swd",
        "frequency": None,
        "reset_mode": None,
        "conn_modifiers": "br=115200 sn=TEST",
        "cli": CLI_PATH,
        "use_elf": False,
        "erase": False,
        "tool_opt": [],
        "system": "",
        "cli_path": str(CLI_PATH),
        "calls": [
            [
                str(CLI_PATH),
                "--connect",
                "port=swd br=115200 sn=TEST",
                "--download",
                RC_KERNEL_HEX,
                "--start",
            ],
        ],
    },
    {
        "port": "swd",
        "frequency": None,
        "reset_mode": None,
        "conn_modifiers": None,
        "cli": CLI_PATH,
        "use_elf": True,
        "erase": False,
        "tool_opt": [],
        "system": "",
        "cli_path": str(CLI_PATH),
        "calls": [
            [
                str(CLI_PATH),
                "--connect",
                "port=swd",
                "--download",
                RC_KERNEL_ELF,
                "--start",
            ],
        ],
    },
    {
        "port": "swd",
        "frequency": None,
        "reset_mode": None,
        "conn_modifiers": None,
        "cli": CLI_PATH,
        "use_elf": False,
        "erase": True,
        "tool_opt": [],
        "system": "",
        "cli_path": str(CLI_PATH),
        "calls": [
            [str(CLI_PATH), "--connect", "port=swd", "--erase", "all",],
            [
                str(CLI_PATH),
                "--connect",
                "port=swd",
                "--download",
                RC_KERNEL_HEX,
                "--start",
            ],
        ],
    },
    {
        "port": "swd",
        "frequency": None,
        "reset_mode": None,
        "conn_modifiers": None,
        "cli": CLI_PATH,
        "use_elf": False,
        "erase": False,
        "tool_opt": ["--skipErase"],
        "system": "",
        "cli_path": str(CLI_PATH),
        "calls": [
            [
                str(CLI_PATH),
                "--connect",
                "port=swd",
                "--skipErase",
                "--download",
                RC_KERNEL_HEX,
                "--start",
            ],
        ],
    },
    {
        "port": "swd",
        "frequency": None,
        "reset_mode": None,
        "conn_modifiers": None,
        "cli": None,
        "use_elf": False,
        "erase": False,
        "tool_opt": [],
        "system": "Linux",
        "cli_path": str(LINUX_CLI_PATH),
        "calls": [
            [
                str(LINUX_CLI_PATH),
                "--connect",
                "port=swd",
                "--download",
                RC_KERNEL_HEX,
                "--start",
            ],
        ],
    },
    {
        "port": "swd",
        "frequency": None,
        "reset_mode": None,
        "conn_modifiers": None,
        "cli": None,
        "use_elf": False,
        "erase": False,
        "tool_opt": [],
        "system": "Darwin",
        "cli_path": str(MACOS_CLI_PATH),
        "calls": [
            [
                str(MACOS_CLI_PATH),
                "--connect",
                "port=swd",
                "--download",
                RC_KERNEL_HEX,
                "--start",
            ],
        ],
    },
    {
        "port": "swd",
        "frequency": None,
        "reset_mode": None,
        "conn_modifiers": None,
        "cli": None,
        "use_elf": False,
        "erase": False,
        "tool_opt": [],
        "system": "Windows",
        "cli_path": str(WINDOWS_CLI_PATH),
        "calls": [
            [
                str(WINDOWS_CLI_PATH),
                "--connect",
                "port=swd",
                "--download",
                RC_KERNEL_HEX,
                "--start",
            ],
        ],
    },
)
"""Test cases."""


@pytest.mark.parametrize("tc", TEST_CASES)
@patch("runners.stm32cubeprogrammer.platform.system")
@patch("runners.stm32cubeprogrammer.Path.home", return_value=HOME_PATH)
@patch("runners.stm32cubeprogrammer.Path.exists", return_value=True)
@patch.dict("runners.stm32cubeprogrammer.os.environ", ENVIRON)
@patch("runners.core.ZephyrBinaryRunner.require")
@patch("runners.stm32cubeprogrammer.STM32CubeProgrammerBinaryRunner.check_call")
def test_stm32cubeprogrammer_init(
    check_call, require, path_exists, path_home, system, tc, runner_config
):
    """Tests that ``STM32CubeProgrammerBinaryRunner`` class can be initialized
    and that ``flash`` command works as expected.
    """

    system.return_value = tc["system"]

    runner = STM32CubeProgrammerBinaryRunner(
        cfg=runner_config,
        port=tc["port"],
        frequency=tc["frequency"],
        reset_mode=tc["reset_mode"],
        conn_modifiers=tc["conn_modifiers"],
        cli=tc["cli"],
        use_elf=tc["use_elf"],
        erase=tc["erase"],
        tool_opt=tc["tool_opt"],
    )

    runner.run("flash")

    require.assert_called_with(tc["cli_path"])
    assert check_call.call_args_list == [call(x) for x in tc["calls"]]


@pytest.mark.parametrize("tc", TEST_CASES)
@patch("runners.stm32cubeprogrammer.platform.system")
@patch("runners.stm32cubeprogrammer.Path.home", return_value=HOME_PATH)
@patch("runners.stm32cubeprogrammer.Path.exists", return_value=True)
@patch.dict("runners.stm32cubeprogrammer.os.environ", ENVIRON)
@patch("runners.core.ZephyrBinaryRunner.require")
@patch("runners.stm32cubeprogrammer.STM32CubeProgrammerBinaryRunner.check_call")
def test_stm32cubeprogrammer_create(
    check_call, require, path_exists, path_home, system, tc, runner_config
):
    """Tests that ``STM32CubeProgrammerBinaryRunner`` class can be created using
    the ``create`` factory method and that ``flash`` command works as expected.
    """

    system.return_value = tc["system"]

    args = ["--port", tc["port"]]
    if tc["frequency"]:
        args.extend(["--frequency", tc["frequency"]])
    if tc["reset_mode"]:
        args.extend(["--reset-mode", tc["reset_mode"]])
    if tc["conn_modifiers"]:
        args.extend(["--conn-modifiers", tc["conn_modifiers"]])
    if tc["cli"]:
        args.extend(["--cli", str(tc["cli"])])
    if tc["use_elf"]:
        args.extend(["--use-elf"])
    if tc["erase"]:
        args.append("--erase")
    if tc["tool_opt"]:
        args.extend(["--tool-opt", " " + tc["tool_opt"][0]])

    parser = argparse.ArgumentParser(allow_abbrev=False)
    STM32CubeProgrammerBinaryRunner.add_parser(parser)
    arg_namespace = parser.parse_args(args)

    runner = STM32CubeProgrammerBinaryRunner.create(runner_config, arg_namespace)
    runner.run("flash")

    require.assert_called_with(tc["cli_path"])
    assert check_call.call_args_list == [call(x) for x in tc["calls"]]
