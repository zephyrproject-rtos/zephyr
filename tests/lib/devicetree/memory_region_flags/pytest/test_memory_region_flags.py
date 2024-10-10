# Copyright (c) 2024, TOKITA Hiroshi <tokita.hiroshi@gmail.com>
# SPDX-License-Identifier: Apache-2.0

import os
import sys
import logging
import re
from twister_harness import DeviceAdapter

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
sys.path.insert(
    0, os.path.join(ZEPHYR_BASE, "scripts", "dts", "python-devicetree", "src")
)
from devicetree import edtlib

logger = logging.getLogger(__name__)


def smart_int(numstr):
    if numstr.startswith("0x"):
        return int(numstr[2:], 16)
    else:
        return int(numstr)


def verify_memory_region_flags(build_dir, label):
    logger.info(label)

    linkercmd = os.path.join(build_dir, "zephyr", "linker.cmd")
    logger.info(linkercmd)

    edt = edtlib.EDT(
        os.path.join(build_dir, "zephyr", "zephyr.dts"),
        [os.path.join(ZEPHYR_BASE, "dts", "bindings")],
    )

    node = edt.label2node[label]
    logger.info(node)

    region = node.props["zephyr,memory-region"].val
    logger.info(region)

    flags = ""
    if "zephyr,memory-region-flags" in node.props.keys():
        flags = "\\(\\s*" + node.props["zephyr,memory-region-flags"].val + "\\s*\\)"

    logger.info(flags)

    origin = node.props["reg"].val[0]
    length = node.props["reg"].val[1]
    pattern = (
        f"{region}\\s*{flags}\\s*:\\s*ORIGIN\\s*=\\s*"
        "\\(?([0-9A-Fa-fx]*)\\)?,\\s*LENGTH\\s*=\\s*\\(?([0-9A-Fa-fx]*)\\)?"
    )

    logger.info(pattern)

    found = False

    with open(linkercmd) as f:
        for line in f:
            m = re.search(pattern, line)
            if m and smart_int(m[1]) == origin and smart_int(m[2]) == length:
                found = True

    assert found


def test_region_rw(dut: DeviceAdapter):
    verify_memory_region_flags(dut.device_config.build_dir, "test_region_rw")


def test_region_ro(dut: DeviceAdapter):
    verify_memory_region_flags(dut.device_config.build_dir, "test_region_ro")


def test_region_no_flags(dut: DeviceAdapter):
    verify_memory_region_flags(dut.device_config.build_dir, "test_region_no_flags")
