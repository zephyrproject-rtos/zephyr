# Copyright (c) 2024 Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: Apache-2.0

import argparse
from unittest.mock import patch, call, mock_open
import pytest
from runners.xsdb import XSDBBinaryRunner
from conftest import RC_KERNEL_ELF


def require_patch(program):
    assert program == 'xsdb'
    return '/usr/bin/xsdb'


# Only keys that differ from their None/absent default need to be given;
# test code below reads the rest via tc.get(key).
TEST_CASES = [
    {
        "expected_cmd": ["xsdb", "default_cfg_path", "elf", RC_KERNEL_ELF],
    },
    {
        "config": "custom_cfg_path",
        "expected_cmd": ["xsdb", "custom_cfg_path", "elf", RC_KERNEL_ELF],
    },
    {
        "bitstream": "bitstream_path",
        "expected_cmd": [
            "xsdb",
            "default_cfg_path",
            "elf",
            RC_KERNEL_ELF,
            "bitstream",
            "bitstream_path",
        ],
    },
    {
        "fsbl": "fsbl_path",
        "expected_cmd": ["xsdb", "default_cfg_path", "elf", RC_KERNEL_ELF, "fsbl", "fsbl_path"],
    },
    {
        "bitstream": "bitstream_path",
        "fsbl": "fsbl_path",
        "expected_cmd": [
            "xsdb",
            "default_cfg_path",
            "elf",
            RC_KERNEL_ELF,
            "bitstream",
            "bitstream_path",
            "fsbl",
            "fsbl_path",
        ],
    },
    {
        "pdi": "pdi_path",
        "expected_cmd": ["xsdb", "default_cfg_path", "elf", RC_KERNEL_ELF, "pdi", "pdi_path"],
    },
    {
        "pdi": "pdi_path",
        "bl31": "bl31_path",
        "expected_cmd": [
            "xsdb",
            "default_cfg_path",
            "elf",
            RC_KERNEL_ELF,
            "pdi",
            "pdi_path",
            "bl31",
            "bl31_path",
        ],
    },
    {
        "pdi": "pdi_path",
        "bl31": "bl31_path",
        "dtb": "dtb_path",
        "expected_cmd": [
            "xsdb",
            "default_cfg_path",
            "elf",
            RC_KERNEL_ELF,
            "pdi",
            "pdi_path",
            "bl31",
            "bl31_path",
            "dtb",
            "dtb_path",
        ],
    },
    {
        "bitstream": "bitstream_path",
        "fsbl": "fsbl_path",
        "bl31": "bl31_path",
        "pmufw": "pmufw_path",
        "expected_cmd": [
            "xsdb",
            "default_cfg_path",
            "elf",
            RC_KERNEL_ELF,
            "bitstream",
            "bitstream_path",
            "fsbl",
            "fsbl_path",
            "bl31",
            "bl31_path",
            "pmufw",
            "pmufw_path",
        ],
    },
    {
        # --param pl_pdi is the generic pass-through for a parameter not
        # covered by a dedicated option.
        "pdi": "pdi_path",
        "params": [("pl_pdi", "pl_pdi_path")],
        "expected_cmd": [
            "xsdb",
            "default_cfg_path",
            "elf",
            RC_KERNEL_ELF,
            "pdi",
            "pdi_path",
            "pl_pdi",
            "pl_pdi_path",
        ],
    },
    {
        "pdi": "pdi_path",
        "params": [("pl_pdi", "pl_pdi_path")],
        "bl31": "bl31_path",
        "expected_cmd": [
            "xsdb",
            "default_cfg_path",
            "elf",
            RC_KERNEL_ELF,
            "pdi",
            "pdi_path",
            "bl31",
            "bl31_path",
            "pl_pdi",
            "pl_pdi_path",
        ],
    },
    {
        "pdi": "pdi_path",
        "params": [("pl_pdi", "pl_pdi_path")],
        "bl31": "bl31_path",
        "dtb": "dtb_path",
        "expected_cmd": [
            "xsdb",
            "default_cfg_path",
            "elf",
            RC_KERNEL_ELF,
            "pdi",
            "pdi_path",
            "bl31",
            "bl31_path",
            "dtb",
            "dtb_path",
            "pl_pdi",
            "pl_pdi_path",
        ],
    },
    {
        # Multiple --param entries sharing a NAME collapse into a single key
        # whose value is a space-joined Tcl list, so boards read them back
        # with a foreach -- e.g. several PL PDIs for partial reconfiguration.
        "pdi": "pdi_path",
        "params": [
            ("pl_pdi", "pl_pdi_path1"),
            ("pl_pdi", "pl_pdi_path2"),
            ("pl_pdi", "pl_pdi_path3"),
        ],
        "bl31": "bl31_path",
        "dtb": "dtb_path",
        "expected_cmd": [
            "xsdb",
            "default_cfg_path",
            "elf",
            RC_KERNEL_ELF,
            "pdi",
            "pdi_path",
            "bl31",
            "bl31_path",
            "dtb",
            "dtb_path",
            "pl_pdi",
            "pl_pdi_path1 pl_pdi_path2 pl_pdi_path3",
        ],
    },
    {
        # --param is fully generic: any NAME a board's xsdb.cfg declares
        # works, not just the ones west happens to know about.
        "pdi": "pdi_path",
        "params": [("some_board_specific_key", "some_value")],
        "expected_cmd": [
            "xsdb",
            "default_cfg_path",
            "elf",
            RC_KERNEL_ELF,
            "pdi",
            "pdi_path",
            "some_board_specific_key",
            "some_value",
        ],
    },
]


@pytest.mark.parametrize("tc", TEST_CASES)
@patch("runners.core.ZephyrBinaryRunner.require", side_effect=require_patch)
@patch("runners.xsdb.os.path.exists", return_value=True)
@patch("runners.xsdb.XSDBBinaryRunner.check_call")
def test_xsdbbinaryrunner_init(check_call, path_exists, require, tc, runner_config):
    '''Test actions using a runner created by constructor.'''
    # Mock the default config path
    with patch("runners.xsdb.os.path.join", return_value="default_cfg_path"):
        runner = XSDBBinaryRunner(
            cfg=runner_config,
            config=tc.get("config"),
            bitstream=tc.get("bitstream"),
            fsbl=tc.get("fsbl"),
            pdi=tc.get("pdi"),
            bl31=tc.get("bl31"),
            dtb=tc.get("dtb"),
            pmufw=tc.get("pmufw"),
            params=tc.get("params"),
        )

    runner.do_run("flash")

    assert require.called
    assert check_call.call_args_list == [call(tc["expected_cmd"])]


@pytest.mark.parametrize("tc", TEST_CASES)
@patch("runners.core.ZephyrBinaryRunner.require", side_effect=require_patch)
@patch("runners.xsdb.os.path.exists", return_value=True)
@patch("runners.xsdb.XSDBBinaryRunner.check_call")
def test_xsdbbinaryrunner_create(check_call, path_exists, require, tc, runner_config):
    '''Test actions using a runner created from action line parameters.'''
    args = []
    if tc.get("config"):
        args.extend(["--config", tc["config"]])
    if tc.get("bitstream"):
        args.extend(["--bitstream", tc["bitstream"]])
    if tc.get("fsbl"):
        args.extend(["--fsbl", tc["fsbl"]])
    if tc.get("pdi"):
        args.extend(["--pdi", tc["pdi"]])
    for name, value in tc.get("params") or []:
        args.extend(["--param", name, value])
    if tc.get("bl31"):
        args.extend(["--bl31", tc["bl31"]])
    if tc.get("dtb"):
        args.extend(["--system-dtb", tc["dtb"]])
    if tc.get("pmufw"):
        args.extend(["--pmufw", tc["pmufw"]])

    parser = argparse.ArgumentParser(allow_abbrev=False)
    XSDBBinaryRunner.add_parser(parser)
    arg_namespace = parser.parse_args(args)

    # Mock the default config path
    with patch("runners.xsdb.os.path.join", return_value="default_cfg_path"):
        runner = XSDBBinaryRunner.create(runner_config, arg_namespace)

    runner.do_run("flash")

    assert require.called
    assert check_call.call_args_list == [call(tc["expected_cmd"])]


@patch("runners.xsdb.os.path.exists")
def test_xsdbbinaryrunner_default_config_prefers_tcl(path_exists, runner_config):
    '''Default config lookup prefers xsdb_cfg.tcl over xsdb.cfg.'''

    def exists_side_effect(path):
        return path.endswith("xsdb_cfg.tcl") or path.endswith("xsdb.cfg")

    path_exists.side_effect = exists_side_effect

    with patch("runners.xsdb.os.path.join", side_effect=lambda *parts: "/".join(parts)):
        runner = XSDBBinaryRunner(cfg=runner_config)

    assert runner.xsdb_cfg_file == f"{runner_config.board_dir}/support/xsdb_cfg.tcl"


@pytest.mark.parametrize("command", ["debug", "debugserver"])
@patch("runners.core.ZephyrBinaryRunner.require", side_effect=require_patch)
@patch("runners.xsdb.os.path.exists", return_value=True)
@patch("runners.xsdb.os.path.isfile", return_value=True)
@patch("runners.xsdb.XSDBBinaryRunner.check_call")
def test_xsdbbinaryrunner_debug(check_call, isfile, path_exists, require, command, runner_config):
    '''Native XSDB debug writes a Tcl params file and launches the interactive script.'''
    from runners import xsdb as xsdb_runner

    def fake_join(*parts):
        if parts[-1] == "xsdb_debug_params.ini":
            return "params.ini"
        return "default_cfg_path"

    open_mock = mock_open()
    with (
        patch("runners.xsdb.os.path.join", side_effect=fake_join),
        patch("builtins.open", open_mock),
    ):
        runner = XSDBBinaryRunner(
            cfg=runner_config,
            pdi="pdi_path",
            bl31="bl31_path",
            hw_server_url="TCP:remote:3121",
        )
        runner.do_run(command)

    assert require.called
    check_call.assert_called_once()
    cmd, kwargs = check_call.call_args
    assert cmd[0] == ["xsdb", "-interactive", xsdb_runner._XSDB_INTERACTIVE_TCL, "params.ini"]
    # Parameters are passed via the generated data file, not the environment.
    assert "env" not in kwargs

    written = "".join(c.args[0] for c in open_mock().write.call_args_list)
    assert "board_cfg = default_cfg_path" in written
    assert f"elf = {RC_KERNEL_ELF}" in written
    assert "hw_server = TCP:remote:3121" in written
    assert "boot_arg = pdi" in written
    assert "boot_arg = pdi_path" in written
    assert "boot_arg = bl31" in written
    assert "boot_arg = bl31_path" in written


@patch("runners.core.ZephyrBinaryRunner.require", side_effect=require_patch)
@patch("runners.xsdb.os.path.exists", return_value=True)
@patch("runners.xsdb.os.path.isfile", return_value=True)
@patch("runners.xsdb.XSDBBinaryRunner.check_call")
def test_xsdb_debug_default_hw_server(check_call, isfile, path_exists, require, runner_config):
    '''Debug without --hw-server must not emit a hw_server parameter.'''
    open_mock = mock_open()
    with (
        patch("runners.xsdb.os.path.join", return_value="default_cfg_path"),
        patch("builtins.open", open_mock),
    ):
        runner = XSDBBinaryRunner(cfg=runner_config, pdi="pdi_path")
        runner.do_run("debug")

    written = "".join(c.args[0] for c in open_mock().write.call_args_list)
    assert "hw_server" not in written
