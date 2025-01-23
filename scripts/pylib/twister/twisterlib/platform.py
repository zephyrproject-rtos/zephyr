#!/usr/bin/env python3
# vim: set syntax=python ts=4 :
#
# Copyright (c) 2018-2022 Intel Corporation
# Copyright (c) 2024 Arm Limited (or its affiliates). All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

import logging
import os
import shutil

import scl
from twisterlib.constants import SUPPORTED_SIMS
from twisterlib.environment import ZEPHYR_BASE

logger = logging.getLogger('twister')
logger.setLevel(logging.DEBUG)


class Simulator:
    """Class representing a simulator"""

    def __init__(self, data: dict[str, str]):
        assert "name" in data
        assert data["name"] in SUPPORTED_SIMS
        self.name = data["name"]
        self.exec = data.get("exec")

    def is_runnable(self) -> bool:
        return not bool(self.exec) or bool(shutil.which(self.exec))

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

    platform_schema = scl.yaml_load(
        os.path.join(ZEPHYR_BASE, "scripts", "schemas", "twister", "platform-schema.yaml")
    )

    def __init__(self):
        """Constructor.

        """

        self.name = ""
        self.aliases = []
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

        self.arch = None
        self.vendor = ""
        self.tier = -1
        self.type = "na"
        self.simulators: list[Simulator] = []
        self.simulation: str = "na"
        self.supported_toolchains = []
        self.env = []
        self.env_satisfied = True
        self.filter_data = dict()
        self.uart = ""
        self.resc = ""
        self.qualifier = None

    def load(self, board, target, aliases, data):
        """Load the platform data from the board data and target data
        board: the board object as per the zephyr build system
        target: the target name of the board as per the zephyr build system
        aliases: list of aliases for the target
        data: the data from the twister.yaml file for the target
        """
        self.name = target
        self.aliases = aliases

        # Get data for various targets and use the main board data as a
        # defauly. Individual variant information will replace the default data
        # provded in the main twister configuration for this board.
        variants = data.get("variants", {})
        variant_data = {}
        for alias in aliases:
            variant_data = variants.get(alias, {})
            if variant_data:
                break

        self.normalized_name = self.name.replace("/", "_")
        self.sysbuild = variant_data.get("sysbuild", data.get("sysbuild", self.sysbuild))
        self.twister = variant_data.get("twister", data.get("twister", self.twister))

        # if no RAM size is specified by the board, take a default of 128K
        self.ram = variant_data.get("ram", data.get("ram", self.ram))
        # if no flash size is specified by the board, take a default of 512K
        self.flash = variant_data.get("flash", data.get("flash", self.flash))

        testing = variant_data.get("testing", data.get("testing", {}))
        self.timeout_multiplier = testing.get("timeout_multiplier", self.timeout_multiplier)
        self.ignore_tags = testing.get("ignore_tags", self.ignore_tags)
        self.only_tags = testing.get("only_tags", self.only_tags)
        self.default = testing.get("default", self.default)
        self.binaries = testing.get("binaries", [])
        renode = testing.get("renode", {})
        self.uart = renode.get("uart", "")
        self.resc = renode.get("resc", "")
        self.supported = set()
        for supp_feature in variant_data.get("supported", data.get("supported", [])):
            for item in supp_feature.split(":"):
                self.supported.add(item)

        self.arch = variant_data.get('arch', data.get('arch', self.arch))
        self.vendor = board.vendor
        self.tier = variant_data.get("tier", data.get("tier", self.tier))
        self.type = variant_data.get('type', data.get('type', self.type))

        self.simulators = [
            Simulator(data) for data in variant_data.get(
                'simulation',
                data.get('simulation', self.simulators)
            )
        ]
        default_sim = self.simulator_by_name(None)
        if default_sim:
            self.simulation = default_sim.name

        self.supported_toolchains = variant_data.get("toolchain", data.get("toolchain", []))
        if self.supported_toolchains is None:
            self.supported_toolchains = []

        support_toolchain_variants = {
          # we don't provide defaults for 'arc' intentionally: some targets can't be built with GNU
          # toolchain ("zephyr", "cross-compile" options) and for some targets we haven't provided
          # MWDT compiler / linker options in corresponding SoC file in Zephyr, so these targets
          # can't be built with ARC MWDT toolchain ("arcmwdt" option) by Zephyr build system Instead
          # for 'arc' we rely on 'toolchain' option in board yaml configuration.
          "arm": ["zephyr", "gnuarmemb", "armclang", "llvm"],
          "arm64": ["zephyr", "cross-compile"],
          "mips": ["zephyr"],
          "nios2": ["zephyr"],
          "riscv": ["zephyr", "cross-compile"],
          "posix": ["host", "llvm"],
          "sparc": ["zephyr"],
          "x86": ["zephyr", "llvm"],
          # Xtensa is not listed on purpose, since there is no single toolchain
          # that is supported on all board targets for xtensa.
        }

        if self.arch in support_toolchain_variants:
            for toolchain in support_toolchain_variants[self.arch]:
                if toolchain not in self.supported_toolchains:
                    self.supported_toolchains.append(toolchain)

        self.env = variant_data.get("env", data.get("env", []))
        self.env_satisfied = True
        for env in self.env:
            if not os.environ.get(env, None):
                self.env_satisfied = False

    def simulator_by_name(self, sim_name: str | None) -> Simulator | None:
        if sim_name:
            return next(filter(lambda s: s.name == sim_name, iter(self.simulators)), None)
        else:
            return next(iter(self.simulators), None)

    def __repr__(self):
        return f"<{self.name} on {self.arch}>"
