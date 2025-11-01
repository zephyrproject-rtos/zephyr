#!/usr/bin/env python3
# vim: set syntax=python ts=4 :
#
# Copyright (c) 2022 Intel Corporation
# Copyright (c) 2025 NXP
# SPDX-License-Identifier: Apache-2.0

import logging
from multiprocessing import Value

import yaml

logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)


class DUT:
    def __init__(
        self,
        id=None,
        serial=None,
        serial_baud=None,
        platform=None,
        product=None,
        serial_pty=None,
        connected=False,
        runner_params=None,
        pre_script=None,
        post_script=None,
        post_flash_script=None,
        runner=None,
    ):
        self.serial = serial
        self.baud = serial_baud or 115200
        self.platform = platform
        self.serial_pty = serial_pty
        self._counter = Value("i", 0)
        self._available = Value("i", 1)
        self.connected = connected
        self.pre_script = pre_script
        self.id = id
        self.product = product
        self.runner = runner
        self.runner_params = runner_params
        self.fixtures = []
        self.post_flash_script = post_flash_script
        self.post_script = post_script
        self.pre_script = pre_script


class HardwareMap:
    def __init__(self):
        self.detected = []
        self.duts = []

    def load(self, map_file):
        # hwm_schema = scl.yaml_load(self.schema_path)
        # duts = scl.yaml_load(map_file)
        with open(map_file, encoding="utf-8") as f:
            duts = yaml.load(f, Loader=yaml.SafeLoader)
        # duts = yaml.load(map_file, Loader=yaml.FullLoader)
        for dut in duts:
            pre_script = dut.get("pre_script")
            post_script = dut.get("post_script")
            post_flash_script = dut.get("post_flash_script")
            platform = dut.get("platform")
            id = dut.get("id")
            runner = dut.get("runner")
            runner_params = dut.get("runner_params")
            serial_pty = dut.get("serial_pty")
            serial = dut.get("serial")
            baud = dut.get("baud", None)
            product = dut.get("product")
            fixtures = dut.get("fixtures", [])
            connected = dut.get("connected") and ((serial or serial_pty) is not None)
            if not connected:
                continue
            new_dut = DUT(
                platform=platform,
                product=product,
                runner=runner,
                runner_params=runner_params,
                id=id,
                serial_pty=serial_pty,
                serial=serial,
                serial_baud=baud,
                connected=connected,
                pre_script=pre_script,
                post_script=post_script,
                post_flash_script=post_flash_script,
            )
            new_dut.fixtures = fixtures
            new_dut.counter = 0
            self.duts.append(new_dut)
