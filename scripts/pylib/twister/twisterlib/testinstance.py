# vim: set syntax=python ts=4 :
#
# Copyright (c) 2018-2022 Intel Corporation
# Copyright 2022 NXP
# SPDX-License-Identifier: Apache-2.0

import os
import hashlib
import random
import logging
import shutil
import glob

from twisterlib.testsuite import TestCase
from twisterlib.error import BuildError
from twisterlib.size_calc import SizeCalculator
from twisterlib.handlers import BinaryHandler, QEMUHandler, DeviceHandler


logger = logging.getLogger('twister')
logger.setLevel(logging.DEBUG)

class TestInstance:
    """Class representing the execution of a particular TestSuite on a platform

    @param test The TestSuite object we want to build/execute
    @param platform Platform object that we want to build and run against
    @param base_outdir Base directory for all test results. The actual
        out directory used is <outdir>/<platform>/<test case name>
    """

    def __init__(self, testsuite, platform, outdir):

        self.testsuite = testsuite
        self.platform = platform

        self.status = None
        self.reason = "Unknown"
        self.metrics = dict()
        self.handler = None
        self.outdir = outdir
        self.execution_time = 0

        self.name = os.path.join(platform.name, testsuite.name)
        self.run_id = self._get_run_id()
        self.build_dir = os.path.join(outdir, platform.name, testsuite.name)

        self.run = False
        self.testcases = []
        self.init_cases()
        self.filters = []
        self.filter_type = None

    def add_filter(self, reason, filter_type):
        self.filters.append({'type': filter_type, 'reason': reason })
        self.status = "filtered"
        self.reason = reason
        self.filter_type = filter_type

    # Fix an issue with copying objects from testsuite, need better solution.
    def init_cases(self):
        for c in self.testsuite.testcases:
            self.add_testcase(c.name, freeform=c.freeform)

    def _get_run_id(self):
        """ generate run id from instance unique identifier and a random
        number"""

        hash_object = hashlib.md5(self.name.encode())
        random_str = f"{random.getrandbits(64)}".encode()
        hash_object.update(random_str)
        return hash_object.hexdigest()

    def add_missing_case_status(self, status, reason=None):
        for case in self.testcases:
            if not case.status:
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

    def is_fixture_available(self, fixtures_cli=None, duts_from_hwmap=None):
        fixture_status = False    # if true: test can be run on a given configuration
        verdict = "NA"
        fixture_required = self.testsuite.harness_config.get('fixture')

        if fixture_required:
            verdict = f"Fixture {fixture_required} is not available."
            # There are 2 places were available fixtures can be defined: from CLI and in hw map
            # Fixtures from CLI applies to all devices
            if fixture_required in fixtures_cli:
                fixture_status = True
                verdict = f"Fixture {fixture_required} is available."
            # If fixture was not given in CLI but a hw map was provided we check if it is available in the hw map
            elif duts_from_hwmap:
                for h in duts_from_hwmap:
                    if h.platform == self.platform.name:
                        if fixture_required in h.fixtures:
                            fixture_status = True
                            verdict = f"Fixture {fixture_required} is available."
        else:
            fixture_status = True
            verdict = "No fixture required"
        return fixture_status, verdict

    def is_harness_supported(self):
        harness_supported = False
        verdict = f"Harness {self.testsuite.harness} is not supported."
        # console harness allows us to run the test and capture data.
        if self.testsuite.harness in [ 'console', 'ztest', 'pytest', 'test']:
            harness_supported = True
            verdict = f"Harness {self.testsuite.harness} is supported."

        return harness_supported, verdict

    def setup_handler(self, env):
        if self.handler:
            return

        options = env.options
        args = []
        handler = None
        if self.platform.simulation == "qemu":
            handler = QEMUHandler(self, "qemu")
            args.append(f"QEMU_PIPE={handler.get_fifo()}")
        elif self.testsuite.type == "unit":
            handler = BinaryHandler(self, "unit")
            handler.binary = os.path.join(self.build_dir, "testbinary")
            if options.enable_coverage:
                args.append("COVERAGE=1")
            handler.call_make_run = False
        elif self.platform.type == "native":
            handler = BinaryHandler(self, "native")
            handler.call_make_run = False
            handler.binary = os.path.join(self.build_dir, "zephyr", "zephyr.exe")
        elif self.platform.simulation == "renode":
            if shutil.which("renode"):
                handler = BinaryHandler(self, "renode")
                handler.pid_fn = os.path.join(self.build_dir, "renode.pid")
        elif self.platform.simulation == "tsim":
            handler = BinaryHandler(self, "tsim")
        elif options.device_testing:
            handler = DeviceHandler(self, "device")
            handler.call_make_run = False
        elif self.platform.simulation == "nsim":
            if shutil.which("nsimdrv"):
                handler = BinaryHandler(self, "nsim")
        elif self.platform.simulation == "mdb-nsim":
            if shutil.which("mdb"):
                handler = BinaryHandler(self, "nsim")
        elif self.platform.simulation == "armfvp":
            handler = BinaryHandler(self, "armfvp")
        elif self.platform.simulation == "xt-sim":
            handler = BinaryHandler(self,  "xt-sim")

        if handler:
            handler.args = args
            handler.options = options
            handler.generator_cmd = env.generator_cmd
            handler.generator = env.generator
            handler.suite_name_check = not options.disable_suite_name_check
        self.handler = handler

    # Global testsuite parameters
    def check_runnable(self, enable_slow=False, fixtures_cli=None, duts_from_hwmap=None):
        verdict = "NA"
        # running on simulators is currently not supported on Windows
        if os.name == 'nt' and self.platform.simulation != 'na':
            verdict = "Simulators not supported on Windows"
            return False, verdict

        # we asked for build-only on the command line
        if self.testsuite.build_only:
            verdict = "test is marked as build-only."
            return False, verdict

        # Do not run slow tests:
        skip_slow = self.testsuite.slow and not enable_slow
        if skip_slow:
            verdict = "test is marked as slow."
            return False, verdict

        harness_supported, verdict = self.is_harness_supported()
        if not harness_supported:
            return False, verdict

        fixture_runnable, verdict = self.is_fixture_available(fixtures_cli, duts_from_hwmap)
        if not fixture_runnable:
            return False, verdict

        target_ready = bool(self.testsuite.type == "unit" or \
                        self.platform.type in ["native", "mcu"] or \
                        self.platform.simulation in ["mdb-nsim", "nsim", "renode", "qemu", "tsim", "armfvp", "xt-sim"])

        if not target_ready:
            verdict = "target type not supported."
            return False, verdict

        if self.platform.simulation == "nsim":
            if not shutil.which("nsimdrv"):
                verdict = "nsimdrv not found."
                return False, verdict

        if self.platform.simulation == "mdb-nsim":
            if not shutil.which("mdb"):
                verdict = "mdb not found."
                return False, verdict

        if self.platform.simulation == "renode":
            if not shutil.which("renode"):
                verdict = "renode not found."
                return False, verdict

        if self.platform.simulation == "tsim":
            if not shutil.which("tsim-leon3"):
                verdict = "tsim-leon3 not found."
                return False, verdict

        return target_ready, verdict

    def create_overlay(self, platform, enable_asan=False, enable_ubsan=False, enable_coverage=False, coverage_platform=[]):
        # Create this in a "twister/" subdirectory otherwise this
        # will pass this overlay to kconfig.py *twice* and kconfig.cmake
        # will silently give that second time precedence over any
        # --extra-args=CONFIG_*
        subdir = os.path.join(self.build_dir, "twister")

        content = ""

        if self.testsuite.extra_configs:
            content = "\n".join(self.testsuite.extra_configs)

        if enable_coverage:
            if platform.name in coverage_platform:
                content = content + "\nCONFIG_COVERAGE=y"
                content = content + "\nCONFIG_COVERAGE_DUMP=y"

        if enable_asan:
            if platform.type == "native":
                content = content + "\nCONFIG_ASAN=y"

        if enable_ubsan:
            if platform.type == "native":
                content = content + "\nCONFIG_UBSAN=y"

        if content:
            os.makedirs(subdir, exist_ok=True)
            file = os.path.join(subdir, "testsuite_extra.conf")
            with open(file, "w") as f:
                f.write(content)

        return content

    def calculate_sizes(self):
        """Get the RAM/ROM sizes of a test case.

        This can only be run after the instance has been executed by
        MakeGenerator, otherwise there won't be any binaries to measure.

        @return A SizeCalculator object
        """
        fns = glob.glob(os.path.join(self.build_dir, "zephyr", "*.elf"))
        fns.extend(glob.glob(os.path.join(self.build_dir, "zephyr", "*.exe")))
        fns = [x for x in fns if '_pre' not in x]
        if len(fns) != 1:
            raise BuildError("Missing/multiple output ELF binary")

        return SizeCalculator(fns[0], self.testsuite.extra_sections)

    def __repr__(self):
        return "<TestSuite %s on %s>" % (self.testsuite.name, self.platform.name)
