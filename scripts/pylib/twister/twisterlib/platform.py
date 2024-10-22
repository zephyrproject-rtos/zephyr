#!/usr/bin/env python3
# vim: set syntax=python ts=4 :
#
# Copyright (c) 2018-2022 Intel Corporation
# Copyright (c) 2024 Arm Limited (or its affiliates). All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

import os
import shutil
from typing import Dict, List, Optional
import scl
from twisterlib.config_parser import TwisterConfigParser
from twisterlib.environment import ZEPHYR_BASE
from twisterlib.constants import SUPPORTED_SIMS


class Simulator:
    """Class representing a simulator"""

    def __init__(self, data: Dict[str, str]):
        assert "name" in data
        assert data["name"] in SUPPORTED_SIMS
        self.name = data["name"]
        self.exec = data.get("exec")

    def is_runnable(self) -> bool:
        return (self.name == 'qemu') or bool(self.exec) and bool(shutil.which(self.exec))

    def __str__(self):
        return f"Simulator(name: {self.name}, exec: {self.exec})"

    def __eq__(self, other):
        if isinstance(other, Simulator):
            return self.name == other.name and self.exec == other.exec
        else:
            return False


class Platform:
    """Class representing metadata for a particular platform

    Maps directly to BOARD when building"""

    platform_schema = scl.yaml_load(os.path.join(ZEPHYR_BASE,
                                                 "scripts", "schemas", "twister", "platform-schema.yaml"))

    def __init__(self):
        """Constructor.

        """

        self.name = ""
        self.normalized_name = ""
        # if sysbuild to be used by default on a given platform
        self.sysbuild = False
        self.twister = True
        # if no RAM size is specified by the board, take a default of 128K
        self.ram = 128

        self.timeout_multiplier = 1.0
        self.ignore_tags = []
        self.only_tags = []
        self.default = False
        # if no flash size is specified by the board, take a default of 512K
        self.flash = 512
        self.supported = set()

        self.arch = ""
        self.vendor = ""
        self.tier = -1
        self.type = "na"
        self.simulators: List[Simulator] = []
        self.simulation: str = "na"
        self.supported_toolchains = []
        self.env = []
        self.env_satisfied = True
        self.filter_data = dict()
        self.uart = ""
        self.resc = ""

    def load(self, platform_file):
        scp = TwisterConfigParser(platform_file, self.platform_schema)
        scp.load()
        data = scp.data

        self.name = data['identifier']
        self.normalized_name = self.name.replace("/", "_")
        self.sysbuild = data.get("sysbuild", False)
        self.twister = data.get("twister", True)
        # if no RAM size is specified by the board, take a default of 128K
        self.ram = data.get("ram", 128)
        testing = data.get("testing", {})
        self.timeout_multiplier = testing.get("timeout_multiplier", 1.0)
        self.ignore_tags = testing.get("ignore_tags", [])
        self.only_tags = testing.get("only_tags", [])
        self.default = testing.get("default", False)
        self.binaries = testing.get("binaries", [])
        renode = testing.get("renode", {})
        self.uart = renode.get("uart", "")
        self.resc = renode.get("resc", "")
        # if no flash size is specified by the board, take a default of 512K
        self.flash = data.get("flash", 512)
        self.supported = set()
        for supp_feature in data.get("supported", []):
            for item in supp_feature.split(":"):
                self.supported.add(item)

        self.arch = data['arch']
        self.vendor = data.get('vendor', '')
        self.tier = data.get("tier", -1)
        self.type = data.get('type', "na")

        self.simulators = [Simulator(data) for data in data.get('simulation', [])]
        default_sim = self.simulator_by_name(None)
        if default_sim:
            self.simulation = default_sim.name

        self.supported_toolchains = data.get("toolchain", [])
        if self.supported_toolchains is None:
            self.supported_toolchains = []

        support_toolchain_variants = {
          # we don't provide defaults for 'arc' intentionally: some targets can't be built with GNU
          # toolchain ("zephyr", "cross-compile", "xtools" options) and for some targets we haven't
          # provided MWDT compiler / linker options in corresponding SoC file in Zephyr, so these
          # targets can't be built with ARC MWDT toolchain ("arcmwdt" option) by Zephyr build system
          # Instead for 'arc' we rely on 'toolchain' option in board yaml configuration.
          "arm": ["zephyr", "gnuarmemb", "xtools", "armclang", "llvm"],
          "arm64": ["zephyr", "cross-compile"],
          "mips": ["zephyr", "xtools"],
          "nios2": ["zephyr", "xtools"],
          "riscv": ["zephyr", "cross-compile"],
          "posix": ["host", "llvm"],
          "sparc": ["zephyr", "xtools"],
          "x86": ["zephyr", "xtools", "llvm"],
          # Xtensa is not listed on purpose, since there is no single toolchain
          # that is supported on all board targets for xtensa.
        }

        if self.arch in support_toolchain_variants:
            for toolchain in support_toolchain_variants[self.arch]:
                if toolchain not in self.supported_toolchains:
                    self.supported_toolchains.append(toolchain)

        self.env = data.get("env", [])
        self.env_satisfied = True
        for env in self.env:
            if not os.environ.get(env, None):
                self.env_satisfied = False

    def simulator_by_name(self, sim_name: Optional[str]) -> Optional[Simulator]:
        if sim_name:
            return next(filter(lambda s: s.name == sim_name, iter(self.simulators)), None)
        else:
            return next(iter(self.simulators), None)

    def __repr__(self):
        return "<%s on %s>" % (self.name, self.arch)
