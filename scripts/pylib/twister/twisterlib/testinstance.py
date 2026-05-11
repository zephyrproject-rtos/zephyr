# vim: set syntax=python ts=4 :
#
# Copyright (c) 2018-2025 Intel Corporation
# Copyright 2022 NXP
# Copyright (c) 2024 Arm Limited (or its affiliates). All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
from __future__ import annotations

import csv
import glob
import hashlib
import logging
import os
import random
import re
from enum import Enum

from twisterlib.constants import (
    SUPPORTED_HARNESSES,
    SUPPORTED_SIMS,
    SUPPORTED_SIMS_IN_PYTEST,
    SUPPORTED_SIMS_WITH_EXEC,
)
from twisterlib.environment import TwisterEnv
from twisterlib.error import BuildError, StatusAttributeError, TwisterException
from twisterlib.handlers import (
    BinaryHandler,
    DeviceHandler,
    Handler,
    QEMUHandler,
    QEMUWinHandler,
    SimulationHandler,
)
from twisterlib.hardwaredata import CompoundHardwareData
from twisterlib.hardwaremap import HardwareMap
from twisterlib.hardwareutil import HardwareReservationManager
from twisterlib.platform import Platform
from twisterlib.size_calc import SizeCalculator
from twisterlib.statuses import TwisterStatus
from twisterlib.testsuite import TestCase, TestSuite

logger = logging.getLogger('twister')


class TestInstance:
    """Class representing the execution of a particular TestSuite on a platform

    @param test The TestSuite object we want to build/execute
    @param platform Platform object that we want to build and run against
    @param base_outdir Base directory for all test results. The actual
        out directory used is <outdir>/<platform>/<test case name>
    """

    __test__ = False

    def __init__(self, testsuite, platform, toolchain, outdir):

        self.testsuite: TestSuite = testsuite
        self.platform: Platform = platform

        self._status = TwisterStatus.NONE
        self.reason = None
        self.metrics = dict()
        self.handler = None
        self.recording = None
        self.coverage = None
        self.coverage_status = None
        self.outdir = outdir
        self.execution_time = 0
        self.build_time = 0
        self.retries = 0
        self.toolchain = toolchain
        self.name = os.path.join(platform.name, toolchain, testsuite.name)
        self.hardware_id: str | None = None
        self.suite_repeat = None
        self.test_repeat = None
        self.test_shuffle = None

        if testsuite.detailed_test_id:
            self.build_dir = os.path.join(
                outdir, platform.normalized_name, self.toolchain.replace('/', '_'), testsuite.name
            )
        else:
            # if suite is not in zephyr,
            # keep only the part after ".." in reconstructed dir structure
            source_dir_rel = testsuite.source_dir_rel.rsplit(os.pardir+os.path.sep, 1)[-1]
            self.build_dir = os.path.join(
                outdir,
                platform.normalized_name,
                self.toolchain.replace('/', '_'),
                source_dir_rel,
                testsuite.name
            )
        self.run_id = None
        self.domains = None
        # Instance need to use sysbuild if a given suite or a platform requires it
        self.sysbuild = testsuite.sysbuild or platform.sysbuild

        self.run = False
        self.testcases: list[TestCase] = []
        self.init_cases()
        self.filters = []
        self.filter_type = None
        self.required_applications = []
        self.required_build_dirs = []
        self.reserved_duts: list[CompoundHardwareData] = []

    def setup_run_id(self):
        self.run_id = self._get_run_id()

    def record(self, recording, fname_csv="recording.csv"):
        if recording:
            if self.recording is None:
                self.recording = recording.copy()
            else:
                self.recording.extend(recording)

            filename = os.path.join(self.build_dir, fname_csv)
            fieldnames = set()
            for r in self.recording:
                fieldnames.update(r)
            with open(filename, 'w') as csvfile:
                cw = csv.DictWriter(csvfile,
                                    fieldnames = sorted(list(fieldnames)),
                                    lineterminator = os.linesep,
                                    quoting = csv.QUOTE_NONNUMERIC)
                cw.writeheader()
                cw.writerows(self.recording)

    @property
    def status(self) -> TwisterStatus:
        return self._status

    @status.setter
    def status(self, value : TwisterStatus) -> None:
        # Check for illegal assignments by value
        try:
            key = value.name if isinstance(value, Enum) else value
            self._status = TwisterStatus[key]
        except KeyError as err:
            raise StatusAttributeError(self.__class__, value) from err

    def add_filter(self, reason, filter_type):
        self.filters.append({'type': filter_type, 'reason': reason })
        self.status = TwisterStatus.FILTER
        self.reason = reason
        self.filter_type = filter_type

    # Fix an issue with copying objects from testsuite, need better solution.
    def init_cases(self):
        for c in self.testsuite.testcases:
            self.add_testcase(c.name, freeform=c.freeform)

    def _get_run_id(self):
        """ generate run id from instance unique identifier and a random
        number
        If exist, get cached run id from previous run."""
        run_id = ""
        run_id_file = os.path.join(self.build_dir, "run_id.txt")
        if os.path.exists(run_id_file):
            with open(run_id_file) as fp:
                run_id = fp.read()
        else:
            hash_object = hashlib.md5(self.name.encode(), usedforsecurity=False)
            random_str = f"{random.getrandbits(64)}".encode()
            hash_object.update(random_str)
            run_id = hash_object.hexdigest()
            os.makedirs(self.build_dir, exist_ok=True)
            with open(run_id_file, 'w+') as fp:
                fp.write(run_id)
        return run_id

    def add_missing_case_status(self, status, reason=None):
        for case in self.testcases:
            if case.status == TwisterStatus.STARTED:
                case.status = TwisterStatus.FAIL
            elif case.status == TwisterStatus.NONE:
                case.status = status
                if reason:
                    case.reason = reason
                else:
                    case.reason = self.reason

    def __getstate__(self):
        d = self.__dict__.copy()
        return d

    def __setstate__(self, d):
        self.__dict__.update(d)

    def __lt__(self, other):
        return self.name < other.name

    def set_case_status_by_name(self, name, status, reason=None):
        tc = self.get_case_or_create(name)
        tc.status = status
        if reason:
            tc.reason = reason
        return tc

    def add_testcase(self, name, freeform=False):
        tc = TestCase(name=name)
        tc.freeform = freeform
        self.testcases.append(tc)
        return tc

    def get_case_by_name(self, name):
        for c in self.testcases:
            if c.name == name:
                return c
        return None

    def get_case_or_create(self, name):
        for c in self.testcases:
            if c.name == name:
                return c

        logger.debug(f"Could not find a matching testcase for {name}")
        tc = TestCase(name=name)
        self.testcases.append(tc)
        return tc

    def setup_handler(self, env: TwisterEnv):
        # only setup once.
        if self.handler:
            return

        options = env.options
        common_args = (options, env.generator_cmd, not options.disable_suite_name_check)
        simulator = self.platform.simulator_by_name(options.sim_name)
        if options.device_testing:
            handler = DeviceHandler(self, "device", *common_args)
            handler.call_make_run = False
            handler.ready = True
        elif simulator:
            if simulator.name == "qemu":
                if os.name != "nt":
                    handler = QEMUHandler(self, "qemu", *common_args)
                else:
                    handler = QEMUWinHandler(self, "qemu", *common_args)
                handler.args.append(f"QEMU_PIPE={handler.get_fifo()}")
                handler.ready = True
            else:
                handler = SimulationHandler(self, simulator.name, *common_args)
                handler.ready = simulator.is_runnable()

        elif self.testsuite.type == "unit":
            handler = BinaryHandler(self, "unit", *common_args)
            handler.binary = os.path.join(self.build_dir, "testbinary")
            if options.enable_coverage:
                handler.args.append("COVERAGE=1")
            handler.call_make_run = False
            handler.ready = True
        else:
            handler = Handler(self, "", *common_args)
            if self.testsuite.harness == "ctest":
                handler.ready = True

        self.handler = handler

    # Global testsuite parameters
    def check_runnable(self,
                       options: TwisterEnv,
                       hardware_map: HardwareMap):

        enable_slow = options.enable_slow
        filter = options.filter
        cli_fixtures = options.fixture
        device_testing = options.device_testing
        simulation = options.sim_name

        simulator = self.platform.simulator_by_name(simulation)
        if os.name == 'nt' and simulator:
            # running on simulators is currently supported only for QEMU on Windows
            if simulator.name not in ('na', 'qemu'):
                return False

            # check presence of QEMU on Windows
            if simulator.name == 'qemu' and 'QEMU_BIN_PATH' not in os.environ:
                return False

        # we asked for build-only on the command line
        if self.testsuite.build_only:
            return False

        # Do not run slow tests:
        skip_slow = self.testsuite.slow and not enable_slow
        if skip_slow:
            return False

        target_ready = bool(self.testsuite.type == "unit" or \
                            self.platform.type == "native" or \
                            self.testsuite.harness == "ctest" or \
                            (simulator and simulator.name in SUPPORTED_SIMS and \
                             simulator.name not in self.testsuite.simulation_exclude) or \
                            device_testing)

        # check if test is runnable in pytest
        if self.testsuite.harness in ['pytest', 'shell', 'power', 'display_capture']:
            target_ready = bool(
                filter == 'runnable' or simulator and simulator.name in SUPPORTED_SIMS_IN_PYTEST
            )

        if filter != 'runnable' and \
                simulator and \
                simulator.name in SUPPORTED_SIMS_WITH_EXEC and \
                not simulator.is_runnable():
            target_ready = False

        if testsuite_runnable := self.testsuite.harness in SUPPORTED_HARNESSES:
            if device_testing:
                testsuite_runnable = HardwareReservationManager(
                    hardware_map, self.platform.name, self.testsuite.harness_config).is_runnable()

            elif fixture := self.testsuite.harness_config.fixture:
                # if we have a fixture that is also being supplied on the
                # command-line, then we need to run the test, not just build it.
                testsuite_runnable = all(f in set(cli_fixtures) for f in fixture)

            elif self.testsuite.harness_config.required_devices:
                # Multi-DUT also allowed for native_sim, but not allowed to use different platforms
                # in required devices
                if simulator and simulator.name == 'native':
                    for req_dev in self.testsuite.harness_config.required_devices:
                        if req_dev.platform and req_dev.platform != self.platform.name:
                            testsuite_runnable = False
                            break
                else:
                    testsuite_runnable = False

        return testsuite_runnable and target_ready

    def create_overlay(
        self,
        platform,
        enable_asan=False,
        enable_ubsan=False,
        enable_coverage=False,
        coverage_platform=None
    ):
        if coverage_platform is None:
            coverage_platform = []
        # Create this in a "twister/" subdirectory otherwise this
        # will pass this overlay to kconfig.py *twice* and kconfig.cmake
        # will silently give that second time precedence over any
        # --extra-args=CONFIG_*
        subdir = os.path.join(self.build_dir, "twister")

        content = ""

        if self.testsuite.extra_configs:
            new_config_list = []
            # some configs might be conditional on arch or platform, see if we
            # have a namespace defined and apply only if the namespace matches.
            # we currently support both arch: and platform:
            for config in self.testsuite.extra_configs:
                cond_config = config.split(":")
                if cond_config[0] == "arch" and len(cond_config) == 3:
                    if self.platform.arch == cond_config[1]:
                        new_config_list.append(cond_config[2])
                elif cond_config[0] == "platform" and len(cond_config) == 3:
                    if self.platform.name == cond_config[1]:
                        new_config_list.append(cond_config[2])
                else:
                    new_config_list.append(config)

            content = "\n".join(new_config_list)


        if self.testsuite.harness_config:
            self.suite_repeat = self.testsuite.harness_config.get('ztest_suite_repeat', None)
            self.test_repeat = self.testsuite.harness_config.get('ztest_test_repeat', None)
            self.test_shuffle = self.testsuite.harness_config.get('ztest_test_shuffle', False)


        # Use suite_repeat and test_repeat values
        if self.suite_repeat or self.test_repeat or self.test_shuffle:
            content +="\nCONFIG_ZTEST_REPEAT=y"
            if self.suite_repeat:
                content += f"\nCONFIG_ZTEST_SUITE_REPEAT_COUNT={self.suite_repeat}"
            if self.test_repeat:
                content += f"\nCONFIG_ZTEST_TEST_REPEAT_COUNT={self.test_repeat}"
            if self.test_shuffle:
                content +="\nCONFIG_ZTEST_SHUFFLE=y"

        if enable_coverage:
            for cp in coverage_platform:
                if cp in platform.aliases:
                    content = content + "\nCONFIG_COVERAGE=y"

        if platform.type == "native":
            if enable_asan:
                content = content + "\nCONFIG_ASAN=y"
            if enable_ubsan:
                content = content + "\nCONFIG_UBSAN=y"

        if content:
            os.makedirs(subdir, exist_ok=True)
            file = os.path.join(subdir, "testsuite_extra.conf")
            with open(file, "w", encoding='utf-8') as f:
                f.write(content)

        return content

    def calculate_sizes(
        self,
        from_buildlog: bool = False,
        generate_warning: bool = True
    ) -> SizeCalculator:
        """Get the RAM/ROM sizes of a test case.

        This can only be run after the instance has been executed by
        MakeGenerator, otherwise there won't be any binaries to measure.

        @return A SizeCalculator object
        """
        elf_filepath = self.get_elf_file()
        buildlog_filepath = self.get_buildlog_file() if from_buildlog else ''
        return SizeCalculator(elf_filename=elf_filepath,
                            extra_sections=self.testsuite.extra_sections,
                            buildlog_filepath=buildlog_filepath,
                            generate_warning=generate_warning)

    def get_elf_file(self) -> str:

        if self.sysbuild:
            build_dir = self.domains.get_default_domain().build_dir
        else:
            build_dir = self.build_dir

        fns = glob.glob(os.path.join(build_dir, "zephyr", "*.elf"))
        fns.extend(glob.glob(os.path.join(build_dir, "testbinary")))
        blocklist = [
                'remapped', # used for xtensa plaforms
                'zefi', # EFI for Zephyr
                'qemu', # elf files generated after running in qemu
                '_pre']
        fns = [x for x in fns if not any(bad in os.path.basename(x) for bad in blocklist)]
        if not fns:
            raise BuildError("Missing output binary")
        elif len(fns) > 1:
            logger.warning(f"multiple ELF files detected: {', '.join(fns)}")
        return fns[0]

    def get_buildlog_file(self) -> str:
        """Get path to build.log file.

        @raises BuildError: Incorrect amount (!=1) of build logs.
        @return: Path to build.log (str).
        """
        buildlog_paths = glob.glob(os.path.join(self.build_dir, "build.log"))
        if len(buildlog_paths) != 1:
            raise BuildError("Missing/multiple build.log file.")
        return buildlog_paths[0]

    def update_reserved_duts_with_required_applications(self):
        if len(self.reserved_duts) < len(self.testsuite.harness_config.required_devices) + 1:
            raise TwisterException("Not enough DUTs reserved for the required devices.")
        for id, req_dev in enumerate(self.testsuite.harness_config.required_devices):
            if not (req_dev.application or req_dev.platform):
                # if neither application nor platform is specified, use the same application
                continue
            if platform_name := req_dev.platform:
                platform_name = platform_name.replace("/", "_")
            else:
                platform_name = self.platform.normalized_name

            application_name = req_dev.application or self.testsuite.id

            pattern = f"{platform_name}/.*/{application_name}"
            for build_dir in self.required_build_dirs:
                if re.search(pattern, build_dir):
                    # found matching build dir
                    break
            else:
                raise TwisterException(
                    "Could not find a build dir for required application "
                    f"{application_name} on platform {platform_name}"
                )

            self.reserved_duts[id + 1].build_dir = build_dir

    def __repr__(self):
        return f"<TestSuite {self.testsuite.name} on {self.platform.name}>"
