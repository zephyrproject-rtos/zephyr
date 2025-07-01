# Copyright (c) 2024 Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: Apache-2.0

import argparse
from unittest.mock import patch, call
import pytest
from runners.xsdb import XSDBBinaryRunner
from conftest import RC_KERNEL_ELF

TEST_CASES = [
    {
        "config": None,
        "bitstream": None,
        "fsbl": None,
        "pdi": None,
        "bl31": None,
        "dtb": None,
        "expected_cmd": ["xsdb", "default_cfg_path", RC_KERNEL_ELF],
    },
    {
        "config": "custom_cfg_path",
        "bitstream": None,
        "fsbl": None,
        "pdi": None,
        "bl31": None,
        "dtb": None,
        "expected_cmd": ["xsdb", "custom_cfg_path", RC_KERNEL_ELF],
    },
    {
        "config": None,
        "bitstream": "bitstream_path",
        "fsbl": None,
        "pdi": None,
        "bl31": None,
        "dtb": None,
        "expected_cmd": ["xsdb", "default_cfg_path", RC_KERNEL_ELF, "bitstream_path"],
    },
    {
        "config": None,
        "bitstream": None,
        "fsbl": "fsbl_path",
        "pdi": None,
        "bl31": None,
        "dtb": None,
        "expected_cmd": ["xsdb", "default_cfg_path", RC_KERNEL_ELF, "fsbl_path"],
    },
    {
        "config": None,
        "bitstream": "bitstream_path",
        "fsbl": "fsbl_path",
        "pdi": None,
        "bl31": None,
        "dtb": None,
        "expected_cmd": ["xsdb", "default_cfg_path", RC_KERNEL_ELF, "bitstream_path", "fsbl_path"],
    },
    {
        "config": None,
        "bitstream": None,
        "fsbl": None,
        "pdi": "pdi_path",
        "bl31": None,
        "dtb": None,
        "expected_cmd": ["xsdb", "default_cfg_path", RC_KERNEL_ELF, "pdi_path"],
    },
    {
        "config": None,
        "bitstream": None,
        "fsbl": None,
        "pdi": "pdi_path",
        "bl31": "bl31_path",
        "dtb": None,
        "expected_cmd": ["xsdb", "default_cfg_path", RC_KERNEL_ELF, "pdi_path", "bl31_path"],
    },
    {
        "config": None,
        "bitstream": None,
        "fsbl": None,
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
            config=tc["config"],
            bitstream=tc["bitstream"],
            fsbl=tc["fsbl"],
            pdi=tc["pdi"],
            bl31=tc["bl31"],
            dtb=tc["dtb"],
        )

    runner.do_run("flash")

    assert check_call.call_args_list == [call(tc["expected_cmd"])]


@pytest.mark.parametrize("tc", TEST_CASES)
@patch("runners.xsdb.os.path.exists", return_value=True)
@patch("runners.xsdb.XSDBBinaryRunner.check_call")
def test_xsdbbinaryrunner_create(check_call, path_exists, tc, runner_config):
    '''Test actions using a runner created from action line parameters.'''
    args = []
    if tc["config"]:
        args.extend(["--config", tc["config"]])
    if tc["bitstream"]:
        args.extend(["--bitstream", tc["bitstream"]])
    if tc["fsbl"]:
        args.extend(["--fsbl", tc["fsbl"]])
    if tc["pdi"]:
        args.extend(["--pdi", tc["pdi"]])
    if tc["bl31"]:
        args.extend(["--bl31", tc["bl31"]])
    if tc["dtb"]:
        args.extend(["--system-dtb", tc["dtb"]])

    parser = argparse.ArgumentParser(allow_abbrev=False)
    XSDBBinaryRunner.add_parser(parser)
    arg_namespace = parser.parse_args(args)

    # Mock the default config path
    with patch("runners.xsdb.os.path.join", return_value="default_cfg_path"):
        runner = XSDBBinaryRunner.create(runner_config, arg_namespace)

    runner.do_run("flash")

    assert check_call.call_args_list == [call(tc["expected_cmd"])]
