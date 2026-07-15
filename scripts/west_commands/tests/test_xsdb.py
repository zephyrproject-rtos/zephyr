# Copyright (c) 2024 Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: Apache-2.0

import argparse
from unittest.mock import patch, call
import pytest
from runners.xsdb import XSDBBinaryRunner
from conftest import RC_KERNEL_ELF

# Only keys that differ from their None/absent default need to be given;
# test code below reads the rest via tc.get(key).
TEST_CASES = [
    {
        "expected_cmd": ["xsdb", "default_cfg_path", RC_KERNEL_ELF],
    },
    {
        "config": "custom_cfg_path",
        "expected_cmd": ["xsdb", "custom_cfg_path", RC_KERNEL_ELF],
    },
    {
        "bitstream": "bitstream_path",
        "expected_cmd": ["xsdb", "default_cfg_path", RC_KERNEL_ELF, "bitstream_path"],
    },
    {
        "fsbl": "fsbl_path",
        "expected_cmd": ["xsdb", "default_cfg_path", RC_KERNEL_ELF, "fsbl_path"],
    },
    {
        "bitstream": "bitstream_path",
        "fsbl": "fsbl_path",
        "expected_cmd": ["xsdb", "default_cfg_path", RC_KERNEL_ELF, "bitstream_path", "fsbl_path"],
    },
    {
        "pdi": "pdi_path",
        "expected_cmd": ["xsdb", "default_cfg_path", RC_KERNEL_ELF, "pdi_path"],
    },
    {
        "pdi": "pdi_path",
        "bl31": "bl31_path",
        "expected_cmd": ["xsdb", "default_cfg_path", RC_KERNEL_ELF, "pdi_path", "bl31_path"],
    },
    {
        "pdi": "pdi_path",
        "bl31": "bl31_path",
        "dtb": "dtb_path",
        "expected_cmd": [
            "xsdb",
            "default_cfg_path",
            RC_KERNEL_ELF,
            "pdi_path",
            "bl31_path",
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
            RC_KERNEL_ELF,
            "bitstream_path",
            "fsbl_path",
            "bl31_path",
            "pmufw_path",
        ],
    },
]


@pytest.mark.parametrize("tc", TEST_CASES)
@patch("runners.xsdb.os.path.exists", return_value=True)
@patch("runners.xsdb.XSDBBinaryRunner.check_call")
def test_xsdbbinaryrunner_init(check_call, path_exists, tc, runner_config):
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
        )

    runner.do_run("flash")

    assert check_call.call_args_list == [call(tc["expected_cmd"])]


@pytest.mark.parametrize("tc", TEST_CASES)
@patch("runners.xsdb.os.path.exists", return_value=True)
@patch("runners.xsdb.XSDBBinaryRunner.check_call")
def test_xsdbbinaryrunner_create(check_call, path_exists, tc, runner_config):
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
