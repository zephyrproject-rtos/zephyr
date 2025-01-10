# vim: set syntax=python ts=4 :
#
# Copyright (c) 2018-2024 Intel Corporation
# Copyright 2022 NXP
# SPDX-License-Identifier: Apache-2.0

import logging
import multiprocessing
import os
import pathlib
import pickle
import queue
import re
import shutil
import subprocess
import sys
import time
import traceback
from math import log10
from multiprocessing import Lock, Process, Value
from multiprocessing.managers import BaseManager

import elftools
import yaml
from colorama import Fore
from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection
from packaging import version
from twisterlib.cmakecache import CMakeCache
from twisterlib.environment import canonical_zephyr_base
from twisterlib.error import BuildError, ConfigurationError, StatusAttributeError
from twisterlib.statuses import TwisterStatus

if version.parse(elftools.__version__) < version.parse('0.24'):
    sys.exit("pyelftools is out of date, need version 0.24 or later")

# Job server only works on Linux for now.
if sys.platform == 'linux':
    from twisterlib.jobserver import GNUMakeJobClient, GNUMakeJobServer, JobClient

from twisterlib.environment import ZEPHYR_BASE

sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/pylib/build_helpers"))
from domains import Domains
from twisterlib.environment import TwisterEnv
from twisterlib.harness import Ctest, HarnessImporter, Pytest
from twisterlib.log_helper import log_command
from twisterlib.platform import Platform
from twisterlib.testinstance import TestInstance
from twisterlib.testplan import change_skip_to_error_if_integration
from twisterlib.testsuite import TestSuite

try:
    from yaml import CSafeLoader as SafeLoader
except ImportError:
    from yaml import SafeLoader

import expr_parser
from anytree import Node, RenderTree

logger = logging.getLogger('twister')
logger.setLevel(logging.DEBUG)


class ExecutionCounter:
    def __init__(self, total=0):
        '''
        Most of the stats are at test instance level
        Except that case statistics are for cases of ALL test instances

        total = yaml test scenarios * applicable platforms
        done := instances that reached report_out stage of the pipeline
        done = filtered_configs + passed + failed + error
        completed = done - filtered_static
        filtered_configs = filtered_runtime + filtered_static

        pass rate = passed / (total - filtered_configs)
        case pass rate = passed_cases / (cases - filtered_cases - skipped_cases)
        '''
        # instances that go through the pipeline
        # updated by report_out()
        self._done = Value('i', 0)

        # iteration
        self._iteration = Value('i', 0)

        # instances that actually executed and passed
        # updated by report_out()
        self._passed = Value('i', 0)

        # instances that are built but not runnable
        # updated by report_out()
        self._notrun = Value('i', 0)

        # static filter + runtime filter + build skipped
        # updated by update_counting_before_pipeline() and report_out()
        self._filtered_configs = Value('i', 0)

        # cmake filter + build skipped
        # updated by report_out()
        self._filtered_runtime = Value('i', 0)

        # static filtered at yaml parsing time
        # updated by update_counting_before_pipeline()
        self._filtered_static = Value('i', 0)

        # updated by report_out() in pipeline
        self._error = Value('i', 0)
        self._failed = Value('i', 0)
        self._skipped = Value('i', 0)

        # initialized to number of test instances
        self._total = Value('i', total)

        #######################################
        # TestCase counters for all instances #
        #######################################
        # updated in report_out
        self._cases = Value('i', 0)

        # updated by update_counting_before_pipeline() and report_out()
        self._skipped_cases = Value('i', 0)
        self._filtered_cases = Value('i', 0)

        # updated by report_out() in pipeline
        self._passed_cases = Value('i', 0)
        self._notrun_cases = Value('i', 0)
        self._failed_cases = Value('i', 0)
        self._error_cases = Value('i', 0)
        self._blocked_cases = Value('i', 0)

        # Incorrect statuses
        self._none_cases = Value('i', 0)
        self._started_cases = Value('i', 0)

        self._warnings = Value('i', 0)

        self.lock = Lock()

    @staticmethod
    def _find_number_length(n):
        if n > 0:
            length = int(log10(n))+1
        elif n == 0:
            length = 1
        else:
            length = int(log10(-n))+2
        return length

    def summary(self):
        selected_cases = self.cases - self.filtered_cases
        selected_configs = self.done - self.filtered_static - self.filtered_runtime


        root = Node("Summary")

        Node(f"Total test suites: {self.total}", parent=root)
        processed_suites = Node(f"Processed test suites: {self.done}", parent=root)
        filtered_suites = Node(
            f"Filtered test suites: {self.filtered_configs}",
            parent=processed_suites
        )
        Node(f"Filtered test suites (static): {self.filtered_static}", parent=filtered_suites)
        Node(f"Filtered test suites (at runtime): {self.filtered_runtime}", parent=filtered_suites)
        selected_suites = Node(f"Selected test suites: {selected_configs}", parent=processed_suites)
        Node(f"Skipped test suites: {self.skipped}", parent=selected_suites)
        Node(f"Passed test suites: {self.passed}", parent=selected_suites)
        Node(f"Built only test suites: {self.notrun}", parent=selected_suites)
        Node(f"Failed test suites: {self.failed}", parent=selected_suites)
        Node(f"Errors in test suites: {self.error}", parent=selected_suites)

        total_cases = Node(f"Total test cases: {self.cases}", parent=root)
        Node(f"Filtered test cases: {self.filtered_cases}", parent=total_cases)
        selected_cases_node = Node(f"Selected test cases: {selected_cases}", parent=total_cases)
        Node(f"Passed test cases: {self.passed_cases}", parent=selected_cases_node)
        Node(f"Skipped test cases: {self.skipped_cases}", parent=selected_cases_node)
        Node(f"Built only test cases: {self.notrun_cases}", parent=selected_cases_node)
        Node(f"Blocked test cases: {self.blocked_cases}", parent=selected_cases_node)
        Node(f"Failed test cases: {self.failed_cases}", parent=selected_cases_node)
        error_cases_node = Node(
            f"Errors in test cases: {self.error_cases}",
            parent=selected_cases_node
        )

        if self.none_cases or self.started_cases:
            Node(
                "The following test case statuses should not appear in a proper execution",
                parent=error_cases_node
            )
        if self.none_cases:
            Node(f"Statusless test cases: {self.none_cases}", parent=error_cases_node)
        if self.started_cases:
            Node(f"Test cases only started: {self.started_cases}", parent=error_cases_node)

        for pre, _, node in RenderTree(root):
            print(f"{pre}{node.name}")

    @property
    def warnings(self):
        with self._warnings.get_lock():
            return self._warnings.value

    @warnings.setter
    def warnings(self, value):
        with self._warnings.get_lock():
            self._warnings.value = value

    def warnings_increment(self, value=1):
        with self._warnings.get_lock():
            self._warnings.value += value

    @property
    def cases(self):
        with self._cases.get_lock():
            return self._cases.value

    @cases.setter
    def cases(self, value):
        with self._cases.get_lock():
            self._cases.value = value

    def cases_increment(self, value=1):
        with self._cases.get_lock():
            self._cases.value += value

    @property
    def skipped_cases(self):
        with self._skipped_cases.get_lock():
            return self._skipped_cases.value

    @skipped_cases.setter
    def skipped_cases(self, value):
        with self._skipped_cases.get_lock():
            self._skipped_cases.value = value

    def skipped_cases_increment(self, value=1):
        with self._skipped_cases.get_lock():
            self._skipped_cases.value += value

    @property
    def filtered_cases(self):
        with self._filtered_cases.get_lock():
            return self._filtered_cases.value

    @filtered_cases.setter
    def filtered_cases(self, value):
        with self._filtered_cases.get_lock():
            self._filtered_cases.value = value

    def filtered_cases_increment(self, value=1):
        with self._filtered_cases.get_lock():
            self._filtered_cases.value += value

    @property
    def passed_cases(self):
        with self._passed_cases.get_lock():
            return self._passed_cases.value

    @passed_cases.setter
    def passed_cases(self, value):
        with self._passed_cases.get_lock():
            self._passed_cases.value = value

    def passed_cases_increment(self, value=1):
        with self._passed_cases.get_lock():
            self._passed_cases.value += value

    @property
    def notrun_cases(self):
        with self._notrun_cases.get_lock():
            return self._notrun_cases.value

    @notrun_cases.setter
    def notrun_cases(self, value):
        with self._notrun.get_lock():
            self._notrun.value = value

    def notrun_cases_increment(self, value=1):
        with self._notrun_cases.get_lock():
            self._notrun_cases.value += value

    @property
    def failed_cases(self):
        with self._failed_cases.get_lock():
            return self._failed_cases.value

    @failed_cases.setter
    def failed_cases(self, value):
        with self._failed_cases.get_lock():
            self._failed_cases.value = value

    def failed_cases_increment(self, value=1):
        with self._failed_cases.get_lock():
            self._failed_cases.value += value

    @property
    def error_cases(self):
        with self._error_cases.get_lock():
            return self._error_cases.value

    @error_cases.setter
    def error_cases(self, value):
        with self._error_cases.get_lock():
            self._error_cases.value = value

    def error_cases_increment(self, value=1):
        with self._error_cases.get_lock():
            self._error_cases.value += value

    @property
    def blocked_cases(self):
        with self._blocked_cases.get_lock():
            return self._blocked_cases.value

    @blocked_cases.setter
    def blocked_cases(self, value):
        with self._blocked_cases.get_lock():
            self._blocked_cases.value = value

    def blocked_cases_increment(self, value=1):
        with self._blocked_cases.get_lock():
            self._blocked_cases.value += value

    @property
    def none_cases(self):
        with self._none_cases.get_lock():
            return self._none_cases.value

    @none_cases.setter
    def none_cases(self, value):
        with self._none_cases.get_lock():
            self._none_cases.value = value

    def none_cases_increment(self, value=1):
        with self._none_cases.get_lock():
            self._none_cases.value += value

    @property
    def started_cases(self):
        with self._started_cases.get_lock():
            return self._started_cases.value

    @started_cases.setter
    def started_cases(self, value):
        with self._started_cases.get_lock():
            self._started_cases.value = value

    def started_cases_increment(self, value=1):
        with self._started_cases.get_lock():
            self._started_cases.value += value

    @property
    def skipped(self):
        with self._skipped.get_lock():
            return self._skipped.value

    @skipped.setter
    def skipped(self, value):
        with self._skipped.get_lock():
            self._skipped.value = value

    def skipped_increment(self, value=1):
        with self._skipped.get_lock():
            self._skipped.value += value

    @property
    def error(self):
        with self._error.get_lock():
            return self._error.value

    @error.setter
    def error(self, value):
        with self._error.get_lock():
            self._error.value = value

    def error_increment(self, value=1):
        with self._error.get_lock():
            self._error.value += value

    @property
    def iteration(self):
        with self._iteration.get_lock():
            return self._iteration.value

    @iteration.setter
    def iteration(self, value):
        with self._iteration.get_lock():
            self._iteration.value = value

    def iteration_increment(self, value=1):
        with self._iteration.get_lock():
            self._iteration.value += value

    @property
    def done(self):
        with self._done.get_lock():
            return self._done.value

    @done.setter
    def done(self, value):
        with self._done.get_lock():
            self._done.value = value

    def done_increment(self, value=1):
        with self._done.get_lock():
            self._done.value += value

    @property
    def passed(self):
        with self._passed.get_lock():
            return self._passed.value

    @passed.setter
    def passed(self, value):
        with self._passed.get_lock():
            self._passed.value = value

    def passed_increment(self, value=1):
        with self._passed.get_lock():
            self._passed.value += value

    @property
    def notrun(self):
        with self._notrun.get_lock():
            return self._notrun.value

    @notrun.setter
    def notrun(self, value):
        with self._notrun.get_lock():
            self._notrun.value = value

    def notrun_increment(self, value=1):
        with self._notrun.get_lock():
            self._notrun.value += value

    @property
    def filtered_configs(self):
        with self._filtered_configs.get_lock():
            return self._filtered_configs.value

    @filtered_configs.setter
    def filtered_configs(self, value):
        with self._filtered_configs.get_lock():
            self._filtered_configs.value = value

    def filtered_configs_increment(self, value=1):
        with self._filtered_configs.get_lock():
            self._filtered_configs.value += value

    @property
    def filtered_static(self):
        with self._filtered_static.get_lock():
            return self._filtered_static.value

    @filtered_static.setter
    def filtered_static(self, value):
        with self._filtered_static.get_lock():
            self._filtered_static.value = value

    def filtered_static_increment(self, value=1):
        with self._filtered_static.get_lock():
            self._filtered_static.value += value

    @property
    def filtered_runtime(self):
        with self._filtered_runtime.get_lock():
            return self._filtered_runtime.value

    @filtered_runtime.setter
    def filtered_runtime(self, value):
        with self._filtered_runtime.get_lock():
            self._filtered_runtime.value = value

    def filtered_runtime_increment(self, value=1):
        with self._filtered_runtime.get_lock():
            self._filtered_runtime.value += value

    @property
    def failed(self):
        with self._failed.get_lock():
            return self._failed.value

    @failed.setter
    def failed(self, value):
        with self._failed.get_lock():
            self._failed.value = value

    def failed_increment(self, value=1):
        with self._failed.get_lock():
            self._failed.value += value

    @property
    def total(self):
        with self._total.get_lock():
            return self._total.value

    @total.setter
    def total(self, value):
        with self._total.get_lock():
            self._total.value = value

    def total_increment(self, value=1):
        with self._total.get_lock():
            self._total.value += value

class CMake:
    config_re = re.compile('(CONFIG_[A-Za-z0-9_]+)[=]\"?([^\"]*)\"?$')
    dt_re = re.compile('([A-Za-z0-9_]+)[=]\"?([^\"]*)\"?$')

    def __init__(self, testsuite: TestSuite, platform: Platform, source_dir, build_dir, jobserver):

        self.cwd = None
        self.capture_output = True

        self.defconfig = {}
        self.cmake_cache = {}

        self.instance = None
        self.testsuite = testsuite
        self.platform = platform
        self.source_dir = source_dir
        self.build_dir = build_dir
        self.log = "build.log"

        self.default_encoding = sys.getdefaultencoding()
        self.jobserver = jobserver

    def parse_generated(self, filter_stages=None):
        self.defconfig = {}
        return {}

    def run_build(self, args=None):
        if args is None:
            args = []

        logger.debug(f"Building {self.source_dir} for {self.platform.name}")

        cmake_args = []
        cmake_args.extend(args)
        cmake = shutil.which('cmake')
        cmd = [cmake] + cmake_args
        kwargs = dict()

        if self.capture_output:
            kwargs['stdout'] = subprocess.PIPE
            # CMake sends the output of message() to stderr unless it's STATUS
            kwargs['stderr'] = subprocess.STDOUT

        if self.cwd:
            kwargs['cwd'] = self.cwd

        start_time = time.time()
        if sys.platform == 'linux':
            p = self.jobserver.popen(cmd, **kwargs)
        else:
            p = subprocess.Popen(cmd, **kwargs)
        logger.debug(f'Running {" ".join(cmd)}')

        out, _ = p.communicate()

        ret = {}
        duration = time.time() - start_time
        self.instance.build_time += duration
        if p.returncode == 0:
            msg = (
                f"Finished building {self.source_dir} for {self.platform.name}"
                f" in {duration:.2f} seconds"
            )
            logger.debug(msg)

            if not self.instance.run:
                self.instance.status = TwisterStatus.NOTRUN
                self.instance.add_missing_case_status(TwisterStatus.NOTRUN, "Test was built only")
            else:
                self.instance.status = TwisterStatus.PASS
            ret = {"returncode": p.returncode}

            if out:
                log_msg = out.decode(self.default_encoding)
                with open(
                    os.path.join(self.build_dir, self.log),
                    "a",
                    encoding=self.default_encoding
                ) as log:
                    log.write(log_msg)
            else:
                return None
        else:
            # A real error occurred, raise an exception
            log_msg = ""
            if out:
                log_msg = out.decode(self.default_encoding)
                with open(
                    os.path.join(self.build_dir, self.log),
                    "a",
                    encoding=self.default_encoding
                ) as log:
                    log.write(log_msg)

            if log_msg:
                pattern = (
                    r"region `(FLASH|ROM|RAM|ICCM|DCCM|SRAM|"
                    r"dram\d_\d_seg|iram\d_\d_seg)' "
                    "overflowed by"
                )
                overflow_found = re.findall(pattern, log_msg)

                imgtool_overflow_found = re.findall(
                    r"Error: Image size \(.*\) \+ trailer \(.*\) exceeds requested size",
                    log_msg
                )
                if overflow_found and not self.options.overflow_as_errors:
                    logger.debug(f"Test skipped due to {overflow_found[0]} Overflow")
                    self.instance.status = TwisterStatus.SKIP
                    self.instance.reason = f"{overflow_found[0]} overflow"
                    change_skip_to_error_if_integration(self.options, self.instance)
                elif imgtool_overflow_found and not self.options.overflow_as_errors:
                    self.instance.status = TwisterStatus.SKIP
                    self.instance.reason = "imgtool overflow"
                    change_skip_to_error_if_integration(self.options, self.instance)
                else:
                    self.instance.status = TwisterStatus.ERROR
                    self.instance.reason = "Build failure"

            ret = {
                "returncode": p.returncode
            }

        return ret

    def run_cmake(self, args="", filter_stages=None):
        if filter_stages is None:
            filter_stages = []

        if not self.options.disable_warnings_as_errors:
            warnings_as_errors = 'y'
            gen_edt_args = "--edtlib-Werror"
        else:
            warnings_as_errors = 'n'
            gen_edt_args = ""

        warning_command = 'CONFIG_COMPILER_WARNINGS_AS_ERRORS'
        if self.instance.sysbuild:
            warning_command = 'SB_' + warning_command

        logger.debug(f"Running cmake on {self.source_dir} for {self.platform.name}")
        cmake_args = [
            f'-B{self.build_dir}',
            f'-DTC_RUNID={self.instance.run_id}',
            f'-DTC_NAME={self.instance.testsuite.name}',
            f'-D{warning_command}={warnings_as_errors}',
            f'-DEXTRA_GEN_EDT_ARGS={gen_edt_args}',
            f'-G{self.env.generator}',
            f'-DPython3_EXECUTABLE={pathlib.Path(sys.executable).as_posix()}'
        ]

        if self.instance.testsuite.harness == 'bsim':
            cmake_args.extend([
                '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON',
                '-DCONFIG_ASSERT=y',
                '-DCONFIG_COVERAGE=y'
            ])

        if self.instance.toolchain:
            cmake_args.append(f'-DZEPHYR_TOOLCHAIN_VARIANT={self.instance.toolchain}')

        # If needed, run CMake using the package_helper script first, to only run
        # a subset of all cmake modules. This output will be used to filter
        # testcases, and the full CMake configuration will be run for
        # testcases that should be built
        if filter_stages:
            cmake_filter_args = [
                f'-DMODULES={",".join(filter_stages)}',
                f'-P{canonical_zephyr_base}/cmake/package_helper.cmake',
            ]

        if self.instance.sysbuild and not filter_stages:
            logger.debug(f"Building {self.source_dir} using sysbuild")
            source_args = [
                f'-S{canonical_zephyr_base}/share/sysbuild',
                f'-DAPP_DIR={self.source_dir}'
            ]
        else:
            source_args = [
                f'-S{self.source_dir}'
            ]
        cmake_args.extend(source_args)

        cmake_args.extend(args)

        cmake_opts = [f'-DBOARD={self.platform.name}']
        cmake_args.extend(cmake_opts)

        if self.instance.testsuite.required_snippets:
            cmake_opts = [
                '-DSNIPPET={}'.format(';'.join(self.instance.testsuite.required_snippets))
            ]
            cmake_args.extend(cmake_opts)

        cmake = shutil.which('cmake')
        cmd = [cmake] + cmake_args

        if filter_stages:
            cmd += cmake_filter_args

        kwargs = dict()

        log_command(logger, "Calling cmake", cmd)

        if self.capture_output:
            kwargs['stdout'] = subprocess.PIPE
            # CMake sends the output of message() to stderr unless it's STATUS
            kwargs['stderr'] = subprocess.STDOUT

        if self.cwd:
            kwargs['cwd'] = self.cwd

        start_time = time.time()
        if sys.platform == 'linux':
            p = self.jobserver.popen(cmd, **kwargs)
        else:
            p = subprocess.Popen(cmd, **kwargs)
        out, _ = p.communicate()

        duration = time.time() - start_time
        self.instance.build_time += duration

        if p.returncode == 0:
            filter_results = self.parse_generated(filter_stages)
            msg = (
                f"Finished running cmake {self.source_dir} for {self.platform.name}"
                f" in {duration:.2f} seconds"
            )
            logger.debug(msg)
            ret = {
                    'returncode': p.returncode,
                    'filter': filter_results
                    }
        else:
            self.instance.status = TwisterStatus.ERROR
            self.instance.reason = "CMake build failure"

            for tc in self.instance.testcases:
                tc.status = self.instance.status

            logger.error(f"CMake build failure: {self.source_dir} for {self.platform.name}")
            ret = {"returncode": p.returncode}

        if out:
            os.makedirs(self.build_dir, exist_ok=True)
            with open(
                os.path.join(self.build_dir, self.log),
                "a",
                encoding=self.default_encoding
            ) as log:
                log_msg = out.decode(self.default_encoding)
                log.write(log_msg)

        return ret


class FilterBuilder(CMake):

    def __init__(self, testsuite: TestSuite, platform: Platform, source_dir, build_dir, jobserver):
        super().__init__(testsuite, platform, source_dir, build_dir, jobserver)

        self.log = "config-twister.log"

    def parse_generated(self, filter_stages=None):
        if filter_stages is None:
            filter_stages = []

        if self.platform.name == "unit_testing":
            return {}

        if self.instance.sysbuild and not filter_stages:
            # Load domain yaml to get default domain build directory
            domain_path = os.path.join(self.build_dir, "domains.yaml")
            domains = Domains.from_file(domain_path)
            logger.debug(f"Loaded sysbuild domain data from {domain_path}")
            self.instance.domains = domains
            domain_build = domains.get_default_domain().build_dir
            cmake_cache_path = os.path.join(domain_build, "CMakeCache.txt")
            defconfig_path = os.path.join(domain_build, "zephyr", ".config")
            edt_pickle = os.path.join(domain_build, "zephyr", "edt.pickle")
        else:
            cmake_cache_path = os.path.join(self.build_dir, "CMakeCache.txt")
            # .config is only available after kconfig stage in cmake.
            # If only dt based filtration is required package helper call won't produce .config
            if not filter_stages or "kconfig" in filter_stages:
                defconfig_path = os.path.join(self.build_dir, "zephyr", ".config")
            # dt is compiled before kconfig,
            # so edt_pickle is available regardless of choice of filter stages
            edt_pickle = os.path.join(self.build_dir, "zephyr", "edt.pickle")


        if not filter_stages or "kconfig" in filter_stages:
            with open(defconfig_path) as fp:
                defconfig = {}
                for line in fp.readlines():
                    m = self.config_re.match(line)
                    if not m:
                        if line.strip() and not line.startswith("#"):
                            sys.stderr.write(f"Unrecognized line {line}\n")
                        continue
                    defconfig[m.group(1)] = m.group(2).strip()

            self.defconfig = defconfig

        cmake_conf = {}
        try:
            cache = CMakeCache.from_file(cmake_cache_path)
        except FileNotFoundError:
            cache = {}

        for k in iter(cache):
            cmake_conf[k.name] = k.value

        self.cmake_cache = cmake_conf

        filter_data = {
            "ARCH": self.platform.arch,
            "PLATFORM": self.platform.name
        }
        filter_data.update(os.environ)
        if not filter_stages or "kconfig" in filter_stages:
            filter_data.update(self.defconfig)
        filter_data.update(self.cmake_cache)

        # Verify that twister's arguments support sysbuild.
        # Twister sysbuild flashing currently only works with west,
        # so --west-flash must be passed.
        if (
            self.instance.sysbuild
            and self.env.options.device_testing
            and self.env.options.west_flash is None
        ):
            logger.warning("Sysbuild test will be skipped. West must be used for flashing.")
            return {
                os.path.join(
                    self.platform.name,
                    self.instance.toolchain,
                    self.testsuite.name
                ): True
            }

        if self.testsuite and self.testsuite.filter:
            try:
                if os.path.exists(edt_pickle):
                    with open(edt_pickle, 'rb') as f:
                        edt = pickle.load(f)
                else:
                    edt = None
                ret = expr_parser.parse(self.testsuite.filter, filter_data, edt)

            except (ValueError, SyntaxError) as se:
                sys.stderr.write(f"Failed processing {self.testsuite.yamlfile}\n")
                raise se

            if not ret:
                return {
                    os.path.join(
                        self.platform.name,
                        self.instance.toolchain,
                        self.testsuite.name
                    ): True
                }
            else:
                return {
                    os.path.join(
                        self.platform.name,
                        self.instance.toolchain,
                        self.testsuite.name
                    ): False
                }
        else:
            self.platform.filter_data = filter_data
            return filter_data


class ProjectBuilder(FilterBuilder):

    def __init__(self, instance: TestInstance, env: TwisterEnv, jobserver, **kwargs):
        super().__init__(
            instance.testsuite,
            instance.platform,
            instance.testsuite.source_dir,
            instance.build_dir,
            jobserver
        )

        self.log = "build.log"
        self.instance = instance
        self.filtered_tests = 0
        self.options = env.options
        self.env = env
        self.duts = None

    @property
    def trace(self) -> bool:
        return self.options.verbose > 2

    def log_info(self, filename, inline_logs, log_testcases=False):
        filename = os.path.abspath(os.path.realpath(filename))
        if inline_logs:
            logger.info(f"{filename:-^100}")

            try:
                with open(filename) as fp:
                    data = fp.read()
            except Exception as e:
                data = f"Unable to read log data ({e!s})\n"

            # Remove any coverage data from the dumped logs
            data = re.sub(
                r"GCOV_COVERAGE_DUMP_START.*GCOV_COVERAGE_DUMP_END",
                "GCOV_COVERAGE_DUMP_START\n...\nGCOV_COVERAGE_DUMP_END",
                data,
                flags=re.DOTALL,
            )
            logger.error(data)

            logger.info(f"{filename:-^100}")

            if log_testcases:
                for tc in self.instance.testcases:
                    if not tc.reason:
                        continue
                    logger.info(
                        f"\n{str(tc.name).center(100, '_')}\n"
                        f"{tc.reason}\n"
                        f"{100*'_'}\n"
                        f"{tc.output}"
                    )
        else:
            logger.error("see: " + Fore.YELLOW + filename + Fore.RESET)

    def log_info_file(self, inline_logs):
        build_dir = self.instance.build_dir
        h_log = f"{build_dir}/handler.log"
        he_log = f"{build_dir}/handler_stderr.log"
        b_log = f"{build_dir}/build.log"
        v_log = f"{build_dir}/valgrind.log"
        d_log = f"{build_dir}/device.log"
        pytest_log = f"{build_dir}/twister_harness.log"

        if os.path.exists(v_log) and "Valgrind" in self.instance.reason:
            self.log_info(f"{v_log}", inline_logs)
        elif os.path.exists(pytest_log) and os.path.getsize(pytest_log) > 0:
            self.log_info(f"{pytest_log}", inline_logs, log_testcases=True)
        elif os.path.exists(h_log) and os.path.getsize(h_log) > 0:
            self.log_info(f"{h_log}", inline_logs)
        elif os.path.exists(he_log) and os.path.getsize(he_log) > 0:
            self.log_info(f"{he_log}", inline_logs)
        elif os.path.exists(d_log) and os.path.getsize(d_log) > 0:
            self.log_info(f"{d_log}", inline_logs)
        else:
            self.log_info(f"{b_log}", inline_logs)


    def _add_to_pipeline(self, pipeline, op: str, additionals: dict=None):
        if additionals is None:
            additionals = {}
        try:
            if op:
                task = dict({'op': op, 'test': self.instance}, **additionals)
                pipeline.put(task)
        # Only possible RuntimeError source here is a mutation of the pipeline during iteration.
        # If that happens, we ought to consider the whole pipeline corrupted.
        except RuntimeError as e:
            logger.error(f"RuntimeError: {e}")
            traceback.print_exc()


    def process(self, pipeline, done, message, lock, results):
        next_op = None
        additionals = {}

        op = message.get('op')

        self.instance.setup_handler(self.env)

        if op == "filter":
            try:
                ret = self.cmake(filter_stages=self.instance.filter_stages)
                if self.instance.status in [TwisterStatus.FAIL, TwisterStatus.ERROR]:
                    next_op = 'report'
                else:
                    # Here we check the dt/kconfig filter results coming from running cmake
                    if self.instance.name in ret['filter'] and ret['filter'][self.instance.name]:
                        logger.debug(f"filtering {self.instance.name}")
                        self.instance.status = TwisterStatus.FILTER
                        self.instance.reason = "runtime filter"
                        results.filtered_runtime_increment()
                        self.instance.add_missing_case_status(TwisterStatus.FILTER)
                        next_op = 'report'
                    else:
                        next_op = 'cmake'
            except StatusAttributeError as sae:
                logger.error(str(sae))
                self.instance.status = TwisterStatus.ERROR
                reason = 'Incorrect status assignment'
                self.instance.reason = reason
                self.instance.add_missing_case_status(TwisterStatus.BLOCK, reason)
                next_op = 'report'
            finally:
                self._add_to_pipeline(pipeline, next_op)

        # The build process, call cmake and build with configured generator
        elif op == "cmake":
            try:
                ret = self.cmake()
                if self.instance.status in [TwisterStatus.FAIL, TwisterStatus.ERROR]:
                    next_op = 'report'
                elif self.options.cmake_only:
                    if self.instance.status == TwisterStatus.NONE:
                        logger.debug(f"CMake only: PASS {self.instance.name}")
                        self.instance.status = TwisterStatus.NOTRUN
                        self.instance.add_missing_case_status(TwisterStatus.NOTRUN, 'CMake only')
                    next_op = 'report'
                else:
                    # Here we check the runtime filter results coming from running cmake
                    if self.instance.name in ret['filter'] and ret['filter'][self.instance.name]:
                        logger.debug(f"filtering {self.instance.name}")
                        self.instance.status = TwisterStatus.FILTER
                        self.instance.reason = "runtime filter"
                        results.filtered_runtime_increment()
                        self.instance.add_missing_case_status(TwisterStatus.FILTER)
                        next_op = 'report'
                    else:
                        next_op = 'build'
            except StatusAttributeError as sae:
                logger.error(str(sae))
                self.instance.status = TwisterStatus.ERROR
                reason = 'Incorrect status assignment'
                self.instance.reason = reason
                self.instance.add_missing_case_status(TwisterStatus.BLOCK, reason)
                next_op = 'report'
            finally:
                self._add_to_pipeline(pipeline, next_op)

        elif op == "build":
            try:
                logger.debug(f"build test: {self.instance.name}")
                ret = self.build()
                if not ret:
                    self.instance.status = TwisterStatus.ERROR
                    self.instance.reason = "Build Failure"
                    next_op = 'report'
                else:
                    # Count skipped cases during build, for example
                    # due to ram/rom overflow.
                    if  self.instance.status == TwisterStatus.SKIP:
                        results.skipped_increment()
                        self.instance.add_missing_case_status(
                            TwisterStatus.SKIP,
                            self.instance.reason
                        )

                    if ret.get('returncode', 1) > 0:
                        self.instance.add_missing_case_status(
                            TwisterStatus.BLOCK,
                            self.instance.reason
                        )
                        next_op = 'report'
                    else:
                        if self.instance.testsuite.harness in ['ztest', 'test']:
                            logger.debug(
                                f"Determine test cases for test instance: {self.instance.name}"
                            )
                            try:
                                self.determine_testcases(results)
                                next_op = 'gather_metrics'
                            except BuildError as e:
                                logger.error(str(e))
                                self.instance.status = TwisterStatus.ERROR
                                self.instance.reason = str(e)
                                next_op = 'report'
                        else:
                            next_op = 'gather_metrics'
            except StatusAttributeError as sae:
                logger.error(str(sae))
                self.instance.status = TwisterStatus.ERROR
                reason = 'Incorrect status assignment'
                self.instance.reason = reason
                self.instance.add_missing_case_status(TwisterStatus.BLOCK, reason)
                next_op = 'report'
            finally:
                self._add_to_pipeline(pipeline, next_op)

        elif op == "gather_metrics":
            try:
                ret = self.gather_metrics(self.instance)
                if not ret or ret.get('returncode', 1) > 0:
                    self.instance.status = TwisterStatus.ERROR
                    self.instance.reason = "Build Failure at gather_metrics."
                    next_op = 'report'
                elif self.instance.run and self.instance.handler.ready:
                    next_op = 'run'
                else:
                    if self.instance.status == TwisterStatus.NOTRUN:
                        run_conditions =  (
                            f"(run:{self.instance.run},"
                            f" handler.ready:{self.instance.handler.ready})"
                        )
                        logger.debug(f"Instance {self.instance.name} can't run {run_conditions}")
                        self.instance.add_missing_case_status(
                            TwisterStatus.NOTRUN,
                            "Nowhere to run"
                        )
                    next_op = 'report'
            except StatusAttributeError as sae:
                logger.error(str(sae))
                self.instance.status = TwisterStatus.ERROR
                reason = 'Incorrect status assignment'
                self.instance.reason = reason
                self.instance.add_missing_case_status(TwisterStatus.BLOCK, reason)
                next_op = 'report'
            finally:
                self._add_to_pipeline(pipeline, next_op)

        # Run the generated binary using one of the supported handlers
        elif op == "run":
            try:
                logger.debug(f"run test: {self.instance.name}")
                self.run()
                logger.debug(f"run status: {self.instance.name} {self.instance.status}")

                # to make it work with pickle
                self.instance.handler.thread = None
                self.instance.handler.duts = None

                next_op = 'report'
                additionals = {
                    "status": self.instance.status,
                    "reason": self.instance.reason
                }
            except StatusAttributeError as sae:
                logger.error(str(sae))
                self.instance.status = TwisterStatus.ERROR
                reason = 'Incorrect status assignment'
                self.instance.reason = reason
                self.instance.add_missing_case_status(TwisterStatus.BLOCK, reason)
                next_op = 'report'
                additionals = {}
            finally:
                self._add_to_pipeline(pipeline, next_op, additionals)

        # Report results and output progress to screen
        elif op == "report":
            try:
                with lock:
                    done.put(self.instance)
                    self.report_out(results)

                if not self.options.coverage:
                    if self.options.prep_artifacts_for_testing:
                        next_op = 'cleanup'
                        additionals = {"mode": "device"}
                    elif self.options.runtime_artifact_cleanup == "pass" and \
                        self.instance.status in [TwisterStatus.PASS, TwisterStatus.NOTRUN]:
                        next_op = 'cleanup'
                        additionals = {"mode": "passed"}
                    elif self.options.runtime_artifact_cleanup == "all":
                        next_op = 'cleanup'
                        additionals = {"mode": "all"}
            except StatusAttributeError as sae:
                logger.error(str(sae))
                self.instance.status = TwisterStatus.ERROR
                reason = 'Incorrect status assignment'
                self.instance.reason = reason
                self.instance.add_missing_case_status(TwisterStatus.BLOCK, reason)
                next_op = None
                additionals = {}
            finally:
                self._add_to_pipeline(pipeline, next_op, additionals)

        elif op == "cleanup":
            try:
                mode = message.get("mode")
                if mode == "device":
                    self.cleanup_device_testing_artifacts()
                elif (
                    mode == "passed"
                    or (mode == "all" and self.instance.reason != "CMake build failure")
                ):
                    self.cleanup_artifacts()
            except StatusAttributeError as sae:
                logger.error(str(sae))
                self.instance.status = TwisterStatus.ERROR
                reason = 'Incorrect status assignment'
                self.instance.reason = reason
                self.instance.add_missing_case_status(TwisterStatus.BLOCK, reason)

    def demangle(self, symbol_name):
        if symbol_name[:2] == '_Z':
            try:
                cpp_filt = subprocess.run(
                    'c++filt',
                    input=symbol_name,
                    text=True,
                    check=True,
                    capture_output=True
                )
                if self.trace:
                    logger.debug(f"Demangle: '{symbol_name}'==>'{cpp_filt.stdout}'")
                return cpp_filt.stdout.strip()
            except Exception as e:
                logger.error(f"Failed to demangle '{symbol_name}': {e}")
        return symbol_name

    def determine_testcases(self, results):
        logger.debug(f"Determine test cases for test suite: {self.instance.testsuite.id}")

        new_ztest_unit_test_regex = re.compile(r"z_ztest_unit_test__([^\s]+?)__([^\s]*)")
        detected_cases = []

        elf_file = self.instance.get_elf_file()
        with open(elf_file, "rb") as elf_fp:
            elf = ELFFile(elf_fp)

            for section in elf.iter_sections():
                if isinstance(section, SymbolTableSection):
                    for sym in section.iter_symbols():
                        # It is only meant for new ztest fx
                        # because only new ztest fx exposes test functions precisely.
                        m_ = new_ztest_unit_test_regex.search(sym.name)
                        if not m_:
                            continue
                        # Demangle C++ symbols
                        m_ = new_ztest_unit_test_regex.search(self.demangle(sym.name))
                        if not m_:
                            continue
                        # The 1st capture group is new ztest suite name.
                        # The 2nd capture group is new ztest unit test name.
                        new_ztest_suite = m_[1]
                        if self.trace and \
                           new_ztest_suite not in self.instance.testsuite.ztest_suite_names:
                            # This can happen if a ZTEST_SUITE name is macro-generated
                            # in the test source files, e.g. based on DT information.
                            logger.debug(
                                f"Unexpected Ztest suite '{new_ztest_suite}' is "
                                f"not present in: {self.instance.testsuite.ztest_suite_names}"
                            )
                        test_func_name = m_[2].replace("test_", "", 1)
                        testcase_id = self.instance.compose_case_name(
                            f"{new_ztest_suite}.{test_func_name}"
                        )
                        detected_cases.append(testcase_id)

        logger.debug(
            f"Test instance {self.instance.name} already has {len(self.instance.testcases)} "
            f"testcase(s) known: {self.instance.testcases}"
        )
        if detected_cases:
            logger.debug(f"Detected {len(detected_cases)} Ztest case(s): "
                         f"[{', '.join(detected_cases)}] in {elf_file}")
            tc_keeper = {
                tc.name: {'status': tc.status, 'reason': tc.reason}
                for tc in self.instance.testcases
            }
            self.instance.testcases.clear()
            self.instance.testsuite.testcases.clear()

            for testcase_id in detected_cases:
                testcase = self.instance.add_testcase(name=testcase_id)
                self.instance.testsuite.add_testcase(name=testcase_id)

                # Keep previous statuses and reasons
                tc_info = tc_keeper.get(testcase_id, {})
                if not tc_info and self.trace:
                    # Also happens when Ztest uses macroses, eg. DEFINE_TEST_VARIANT
                    logger.debug(f"Ztest case '{testcase_id}' discovered for "
                                 f"'{self.instance.testsuite.source_dir_rel}' "
                                 f"with {list(tc_keeper)}")
                testcase.status = tc_info.get('status', TwisterStatus.NONE)
                testcase.reason = tc_info.get('reason')


    def cleanup_artifacts(self, additional_keep: list[str] = None):
        if additional_keep is None:
            additional_keep = []
        logger.debug(f"Cleaning up {self.instance.build_dir}")
        allow = [
            os.path.join('zephyr', '.config'),
            'handler.log',
            'handler_stderr.log',
            'build.log',
            'device.log',
            'recording.csv',
            'rom.json',
            'ram.json',
            # below ones are needed to make --test-only work as well
            'Makefile',
            'CMakeCache.txt',
            'build.ninja',
            os.path.join('CMakeFiles', 'rules.ninja')
            ]

        allow += additional_keep

        if self.options.runtime_artifact_cleanup == 'all':
            allow += [os.path.join('twister', 'testsuite_extra.conf')]

        allow = [os.path.join(self.instance.build_dir, file) for file in allow]

        for dirpath, dirnames, filenames in os.walk(self.instance.build_dir, topdown=False):
            for name in filenames:
                path = os.path.join(dirpath, name)
                if path not in allow:
                    os.remove(path)
            # Remove empty directories and symbolic links to directories
            for dir in dirnames:
                path = os.path.join(dirpath, dir)
                if os.path.islink(path):
                    os.remove(path)
                elif not os.listdir(path):
                    os.rmdir(path)

    def cleanup_device_testing_artifacts(self):
        logger.debug(f"Cleaning up for Device Testing {self.instance.build_dir}")

        files_to_keep = self._get_binaries()
        files_to_keep.append(os.path.join('zephyr', 'runners.yaml'))

        if self.instance.sysbuild:
            files_to_keep.append('domains.yaml')
            for domain in self.instance.domains.get_domains():
                files_to_keep += self._get_artifact_allow_list_for_domain(domain.name)

        self.cleanup_artifacts(files_to_keep)

        self._sanitize_files()

    def _get_artifact_allow_list_for_domain(self, domain: str) -> list[str]:
        """
        Return a list of files needed to test a given domain.
        """
        allow = [
            os.path.join(domain, 'build.ninja'),
            os.path.join(domain, 'CMakeCache.txt'),
            os.path.join(domain, 'CMakeFiles', 'rules.ninja'),
            os.path.join(domain, 'Makefile'),
            os.path.join(domain, 'zephyr', '.config'),
            os.path.join(domain, 'zephyr', 'runners.yaml')
            ]
        return allow

    def _get_binaries(self) -> list[str]:
        """
        Get list of binaries paths (absolute or relative to the
        self.instance.build_dir), basing on information from platform.binaries
        or runners.yaml. If they are not found take default binaries like
        "zephyr/zephyr.hex" etc.
        """
        binaries: list[str] = []

        platform = self.instance.platform
        if platform.binaries:
            for binary in platform.binaries:
                binaries.append(os.path.join('zephyr', binary))

        # Get binaries for a single-domain build
        binaries += self._get_binaries_from_runners()
        # Get binaries in the case of a multiple-domain build
        if self.instance.sysbuild:
            for domain in self.instance.domains.get_domains():
                binaries += self._get_binaries_from_runners(domain.name)

        # if binaries was not found in platform.binaries and runners.yaml take default ones
        if len(binaries) == 0:
            binaries = [
                os.path.join('zephyr', 'zephyr.hex'),
                os.path.join('zephyr', 'zephyr.bin'),
                os.path.join('zephyr', 'zephyr.elf'),
                os.path.join('zephyr', 'zephyr.exe'),
            ]
        return binaries

    def _get_binaries_from_runners(self, domain='') -> list[str]:
        """
        Get list of binaries paths (absolute or relative to the
        self.instance.build_dir) from runners.yaml file. May be used for
        multiple-domain builds by passing in one domain at a time.
        """

        runners_file_path: str = os.path.join(self.instance.build_dir,
                                              domain, 'zephyr', 'runners.yaml')
        if not os.path.exists(runners_file_path):
            return []

        with open(runners_file_path) as file:
            runners_content: dict = yaml.load(file, Loader=SafeLoader)

        if 'config' not in runners_content:
            return []

        runners_config: dict = runners_content['config']
        binary_keys: list[str] = ['elf_file', 'hex_file', 'bin_file']

        binaries: list[str] = []
        for binary_key in binary_keys:
            binary_path = runners_config.get(binary_key)
            if binary_path is None:
                continue
            if os.path.isabs(binary_path):
                binaries.append(binary_path)
            else:
                binaries.append(os.path.join(domain, 'zephyr', binary_path))

        return binaries

    def _sanitize_files(self):
        """
        Sanitize files to make it possible to flash those file on different
        computer/system.
        """
        self._sanitize_runners_file()
        self._sanitize_zephyr_base_from_files()

    def _sanitize_runners_file(self):
        """
        Replace absolute paths of binary files for relative ones. The base
        directory for those files is f"{self.instance.build_dir}/zephyr"
        """
        runners_dir_path: str = os.path.join(self.instance.build_dir, 'zephyr')
        runners_file_path: str = os.path.join(runners_dir_path, 'runners.yaml')
        if not os.path.exists(runners_file_path):
            return

        with open(runners_file_path) as file:
            runners_content_text = file.read()
            runners_content_yaml: dict = yaml.load(runners_content_text, Loader=SafeLoader)

        if 'config' not in runners_content_yaml:
            return

        runners_config: dict = runners_content_yaml['config']
        binary_keys: list[str] = ['elf_file', 'hex_file', 'bin_file']

        for binary_key in binary_keys:
            binary_path = runners_config.get(binary_key)
            # sanitize only paths which exist and are absolute
            if binary_path is None or not os.path.isabs(binary_path):
                continue
            binary_path_relative = os.path.relpath(binary_path, start=runners_dir_path)
            runners_content_text = runners_content_text.replace(binary_path, binary_path_relative)

        with open(runners_file_path, 'w') as file:
            file.write(runners_content_text)

    def _sanitize_zephyr_base_from_files(self):
        """
        Remove Zephyr base paths from selected files.
        """
        files_to_sanitize = [
            'CMakeCache.txt',
            os.path.join('zephyr', 'runners.yaml'),
        ]
        for file_path in files_to_sanitize:
            file_path = os.path.join(self.instance.build_dir, file_path)
            if not os.path.exists(file_path):
                continue

            with open(file_path) as file:
                data = file.read()

            # add trailing slash at the end of canonical_zephyr_base if it does not exist:
            path_to_remove = os.path.join(canonical_zephyr_base, "")
            data = data.replace(path_to_remove, "")

            with open(file_path, 'w') as file:
                file.write(data)

    @staticmethod
    def _add_instance_testcases_to_status_counts(instance, results, decrement=False):
        increment_value = -1 if decrement else 1
        for tc in instance.testcases:
            match tc.status:
                case TwisterStatus.PASS:
                    results.passed_cases_increment(increment_value)
                case TwisterStatus.NOTRUN:
                    results.notrun_cases_increment(increment_value)
                case TwisterStatus.BLOCK:
                    results.blocked_cases_increment(increment_value)
                case TwisterStatus.SKIP:
                    results.skipped_cases_increment(increment_value)
                case TwisterStatus.FILTER:
                    results.filtered_cases_increment(increment_value)
                case TwisterStatus.ERROR:
                    results.error_cases_increment(increment_value)
                case TwisterStatus.FAIL:
                    results.failed_cases_increment(increment_value)
                # Statuses that should not appear.
                # Crashing Twister at this point would be counterproductive,
                # but having those statuses in this part of processing is an error.
                case TwisterStatus.NONE:
                    results.none_cases_increment(increment_value)
                    logger.warning(f'A None status detected in instance {instance.name},'
                                 f' test case {tc.name}.')
                    results.warnings_increment(1)
                case TwisterStatus.STARTED:
                    results.started_cases_increment(increment_value)
                    logger.warning(f'A started status detected in instance {instance.name},'
                                 f' test case {tc.name}.')
                    results.warnings_increment(1)
                case _:
                    logger.warning(
                        f'An unknown status "{tc.status}" detected in instance {instance.name},'
                        f' test case {tc.name}.'
                    )
                    results.warnings_increment(1)


    def report_out(self, results):
        total_to_do = results.total - results.filtered_static
        total_tests_width = len(str(total_to_do))
        results.done_increment()
        instance = self.instance
        if results.iteration == 1:
            results.cases_increment(len(instance.testcases))

        self._add_instance_testcases_to_status_counts(instance, results)

        status = (
            f'{TwisterStatus.get_color(instance.status)}{str.upper(instance.status)}{Fore.RESET}'
        )

        if instance.status in [TwisterStatus.ERROR, TwisterStatus.FAIL]:
            if instance.status == TwisterStatus.ERROR:
                results.error_increment()
            else:
                results.failed_increment()
            if self.options.verbose:
                status += " " + instance.reason
            else:
                logger.error(
                    f"{instance.platform.name:<25} {instance.testsuite.name:<50}"
                    f" {status}: {instance.reason}"
                )
            if not self.options.verbose:
                self.log_info_file(self.options.inline_logs)
        elif instance.status == TwisterStatus.SKIP:
            results.skipped_increment()
        elif instance.status == TwisterStatus.FILTER:
            results.filtered_configs_increment()
        elif instance.status == TwisterStatus.PASS:
            results.passed_increment()
        elif instance.status == TwisterStatus.NOTRUN:
            results.notrun_increment()
        else:
            logger.debug(f"Unknown status = {instance.status}")
            status = Fore.YELLOW + "UNKNOWN" + Fore.RESET

        if self.options.verbose:
            if self.options.cmake_only:
                more_info = "cmake"
            elif instance.status in [TwisterStatus.SKIP, TwisterStatus.FILTER]:
                more_info = instance.reason
            else:
                if instance.handler.ready and instance.run:
                    more_info = instance.handler.type_str
                    htime = instance.execution_time
                    if instance.dut:
                        more_info += f": {instance.dut},"
                    if htime:
                        more_info += f" {htime:.3f}s"
                else:
                    more_info = "build"

                if ( instance.status in [TwisterStatus.ERROR, TwisterStatus.FAIL]
                     and hasattr(self.instance.handler, 'seed')
                     and self.instance.handler.seed is not None ):
                    more_info += "/seed: " + str(self.options.seed)
                if instance.toolchain:
                    more_info += f" <{instance.toolchain}>"
            logger.info(
                f"{results.done - results.filtered_static:>{total_tests_width}}/{total_to_do}"
                f" {instance.platform.name:<25} {instance.testsuite.name:<50}"
                f" {status} ({more_info})"
            )

            if self.options.verbose > 1:
                for tc in self.instance.testcases:
                    color = TwisterStatus.get_color(tc.status)
                    logger.info(f'    {" ":<{total_tests_width+25+4}} {tc.name:<75} '
                                f'{color}{str.upper(tc.status.value):<12}{Fore.RESET}'
                                f'{" " + tc.reason if tc.reason else ""}')

            if instance.status in [TwisterStatus.ERROR, TwisterStatus.FAIL]:
                self.log_info_file(self.options.inline_logs)
        else:
            completed_perc = 0
            if total_to_do > 0:
                completed_perc = int(
                    (float(results.done - results.filtered_static) / total_to_do) * 100
                )

            unfiltered = results.done - results.filtered_static
            complete_section = (
                f"{TwisterStatus.get_color(TwisterStatus.PASS)}"
                f"{unfiltered:>4}/{total_to_do:>4}"
                f"{Fore.RESET}  {completed_perc:>2}%"
            )
            notrun_section = (
                f"{TwisterStatus.get_color(TwisterStatus.NOTRUN)}{results.notrun:>4}{Fore.RESET}"
            )
            filtered_section_color = (
                TwisterStatus.get_color(TwisterStatus.SKIP)
                if results.filtered_configs > 0
                else Fore.RESET
            )
            filtered_section = (
                f"{filtered_section_color}{results.filtered_configs:>4}{Fore.RESET}"
            )
            failed_section_color = (
                TwisterStatus.get_color(TwisterStatus.FAIL) if results.failed > 0 else Fore.RESET
            )
            failed_section = (
                f"{failed_section_color}{results.failed:>4}{Fore.RESET}"
            )
            error_section_color = (
                TwisterStatus.get_color(TwisterStatus.ERROR) if results.error > 0 else Fore.RESET
            )
            error_section = (
                f"{error_section_color}{results.error:>4}{Fore.RESET}"
            )
            sys.stdout.write(
                f"INFO    - Total complete: {complete_section}"
                f"  built (not run): {notrun_section},"
                f" filtered: {filtered_section},"
                f" failed: {failed_section},"
                f" error: {error_section}\r"
            )
        sys.stdout.flush()

    @staticmethod
    def cmake_assemble_args(extra_args, handler, extra_conf_files, extra_overlay_confs,
                            extra_dtc_overlay_files, cmake_extra_args,
                            build_dir):
        # Retain quotes around config options
        config_options = [arg for arg in extra_args if arg.startswith("CONFIG_")]
        args = [arg for arg in extra_args if not arg.startswith("CONFIG_")]

        args_expanded = ["-D{}".format(a.replace('"', '\"')) for a in config_options]

        if handler.ready:
            args.extend(handler.args)

        if extra_conf_files:
            args.append(f"CONF_FILE=\"{';'.join(extra_conf_files)}\"")

        if extra_dtc_overlay_files:
            args.append(f"DTC_OVERLAY_FILE=\"{';'.join(extra_dtc_overlay_files)}\"")

        # merge overlay files into one variable
        overlays = extra_overlay_confs.copy()

        additional_overlay_path = os.path.join(
            build_dir, "twister", "testsuite_extra.conf"
        )
        if os.path.exists(additional_overlay_path):
            overlays.append(additional_overlay_path)

        if overlays:
            args.append(f"OVERLAY_CONFIG=\"{' '.join(overlays)}\"")

        # Build the final argument list
        args_expanded.extend(["-D{}".format(a.replace('"', '\"')) for a in cmake_extra_args])
        args_expanded.extend(["-D{}".format(a.replace('"', '')) for a in args])

        return args_expanded

    def cmake(self, filter_stages=None):
        if filter_stages is None:
            filter_stages = []
        args = []
        for va in self.testsuite.extra_args.copy():
            cond_args = va.split(":")
            if cond_args[0] == "arch" and len(cond_args) == 3:
                if self.instance.platform.arch == cond_args[1]:
                    args.append(cond_args[2])
            elif cond_args[0] == "platform" and len(cond_args) == 3:
                if self.instance.platform.name == cond_args[1]:
                    args.append(cond_args[2])
            elif cond_args[0] == "simulation" and len(cond_args) == 3:
                if self.instance.platform.simulation == cond_args[1]:
                    args.append(cond_args[2])
            else:
                if cond_args[0] in ["arch", "platform", "simulation"]:
                    logger.warning(f"Unexpected extra_args: {va}")
                args.append(va)


        args = self.cmake_assemble_args(
            args,
            self.instance.handler,
            self.testsuite.extra_conf_files,
            self.testsuite.extra_overlay_confs,
            self.testsuite.extra_dtc_overlay_files,
            self.options.extra_args, # CMake extra args
            self.instance.build_dir,
        )
        return self.run_cmake(args,filter_stages)

    def build(self):
        harness = HarnessImporter.get_harness(self.instance.testsuite.harness.capitalize())
        build_result = self.run_build(['--build', self.build_dir])
        try:
            if harness:
                harness.instance = self.instance
                harness.build()
        except ConfigurationError as error:
            self.instance.status = TwisterStatus.ERROR
            self.instance.reason = str(error)
            logger.error(self.instance.reason)
            return
        return build_result

    def run(self):

        instance = self.instance

        if instance.handler.ready:
            logger.debug(f"Reset instance status from '{instance.status}' to None before run.")
            instance.status = TwisterStatus.NONE

            if instance.handler.type_str == "device":
                instance.handler.duts = self.duts

            if(self.options.seed is not None and instance.platform.name.startswith("native_")):
                self.parse_generated()
                if('CONFIG_FAKE_ENTROPY_NATIVE_POSIX' in self.defconfig and
                    self.defconfig['CONFIG_FAKE_ENTROPY_NATIVE_POSIX'] == 'y'):
                    instance.handler.seed = self.options.seed

            if self.options.extra_test_args and instance.platform.arch == "posix":
                instance.handler.extra_test_args = self.options.extra_test_args

            harness = HarnessImporter.get_harness(instance.testsuite.harness.capitalize())
            try:
                harness.configure(instance)
            except ConfigurationError as error:
                instance.status = TwisterStatus.ERROR
                instance.reason = str(error)
                logger.error(instance.reason)
                return
            #
            if isinstance(harness, Pytest):
                harness.pytest_run(instance.handler.get_test_timeout())
            elif isinstance(harness, Ctest):
                harness.ctest_run(instance.handler.get_test_timeout())
            else:
                instance.handler.handle(harness)

        sys.stdout.flush()

    def gather_metrics(self, instance: TestInstance):
        build_result = {"returncode": 0}
        if self.options.create_rom_ram_report:
            build_result = self.run_build(['--build', self.build_dir, "--target", "footprint"])
        if self.options.enable_size_report and not self.options.cmake_only:
            self.calc_size(instance=instance, from_buildlog=self.options.footprint_from_buildlog)
        else:
            instance.metrics["used_ram"] = 0
            instance.metrics["used_rom"] = 0
            instance.metrics["available_rom"] = 0
            instance.metrics["available_ram"] = 0
            instance.metrics["unrecognized"] = []
        return build_result

    @staticmethod
    def calc_size(instance: TestInstance, from_buildlog: bool):
        if instance.status not in [TwisterStatus.ERROR, TwisterStatus.FAIL, TwisterStatus.SKIP]:
            if instance.platform.type not in ["native", "qemu", "unit"]:
                generate_warning = bool(instance.platform.type == "mcu")
                size_calc = instance.calculate_sizes(
                    from_buildlog=from_buildlog,
                    generate_warning=generate_warning
                )
                instance.metrics["used_ram"] = size_calc.get_used_ram()
                instance.metrics["used_rom"] = size_calc.get_used_rom()
                instance.metrics["available_rom"] = size_calc.get_available_rom()
                instance.metrics["available_ram"] = size_calc.get_available_ram()
                instance.metrics["unrecognized"] = size_calc.unrecognized_sections()
            else:
                instance.metrics["used_ram"] = 0
                instance.metrics["used_rom"] = 0
                instance.metrics["available_rom"] = 0
                instance.metrics["available_ram"] = 0
                instance.metrics["unrecognized"] = []
            instance.metrics["handler_time"] = instance.execution_time

class TwisterRunner:

    def __init__(self, instances, suites, env=None) -> None:
        self.pipeline = None
        self.options = env.options
        self.env = env
        self.instances = instances
        self.suites = suites
        self.duts = None
        self.jobs = 1
        self.results = None
        self.jobserver = None

    def run(self):

        retries = self.options.retry_failed + 1

        BaseManager.register('LifoQueue', queue.LifoQueue)
        manager = BaseManager()
        manager.start()

        self.results = ExecutionCounter(total=len(self.instances))
        self.iteration = 0
        pipeline = manager.LifoQueue()
        done_queue = manager.LifoQueue()

        # Set number of jobs
        if self.options.jobs:
            self.jobs = self.options.jobs
        elif self.options.build_only:
            self.jobs = multiprocessing.cpu_count() * 2
        else:
            self.jobs = multiprocessing.cpu_count()

        if sys.platform == "linux":
            if os.name == 'posix':
                self.jobserver = GNUMakeJobClient.from_environ(jobs=self.options.jobs)
                if not self.jobserver:
                    self.jobserver = GNUMakeJobServer(self.jobs)
                elif self.jobserver.jobs:
                    self.jobs = self.jobserver.jobs
            # TODO: Implement this on windows/mac also
            else:
                self.jobserver = JobClient()

            logger.info(f"JOBS: {self.jobs}")

        self.update_counting_before_pipeline()

        while True:
            self.results.iteration_increment()

            if self.results.iteration > 1:
                logger.info(f"{self.results.iteration} Iteration:")
                time.sleep(self.options.retry_interval)  # waiting for the system to settle down
                self.results.done = self.results.total - self.results.failed
                self.results.failed = 0
                if self.options.retry_build_errors:
                    self.results.error = 0
                    self.results.done -= self.results.error
            else:
                self.results.done = self.results.filtered_static

            self.execute(pipeline, done_queue)

            while True:
                try:
                    inst = done_queue.get_nowait()
                except queue.Empty:
                    break
                else:
                    inst.metrics.update(self.instances[inst.name].metrics)
                    inst.metrics["handler_time"] = inst.execution_time
                    inst.metrics["unrecognized"] = []
                    self.instances[inst.name] = inst

            print("")

            retry_errors = False
            if self.results.error and self.options.retry_build_errors:
                retry_errors = True

            retries = retries - 1
            if retries == 0 or ( self.results.failed == 0 and not retry_errors):
                break

        self.show_brief()

    def update_counting_before_pipeline(self):
        '''
        Updating counting before pipeline is necessary because statically filterd
        test instance never enter the pipeline. While some pipeline output needs
        the static filter stats. So need to prepare them before pipline starts.
        '''
        for instance in self.instances.values():
            if instance.status == TwisterStatus.FILTER and instance.reason != 'runtime filter':
                self.results.filtered_static_increment()
                self.results.filtered_configs_increment()
                self.results.filtered_cases_increment(len(instance.testsuite.testcases))
                self.results.cases_increment(len(instance.testsuite.testcases))
            elif instance.status == TwisterStatus.ERROR:
                self.results.error_increment()

    def show_brief(self):
        logger.info(
            f"{len(self.suites)} test scenarios ({len(self.instances)} configurations) selected,"
            f" {self.results.filtered_configs} configurations filtered"
            f" ({self.results.filtered_static} by static filter,"
            f" {self.results.filtered_configs - self.results.filtered_static} at runtime)."
        )

    def add_tasks_to_queue(
        self,
        pipeline,
        build_only=False,
        test_only=False,
        retry_build_errors=False
    ):
        for instance in self.instances.values():
            if build_only:
                instance.run = False

            no_retry_statuses = [
                TwisterStatus.PASS,
                TwisterStatus.SKIP,
                TwisterStatus.FILTER,
                TwisterStatus.NOTRUN
            ]
            if not retry_build_errors:
                no_retry_statuses.append(TwisterStatus.ERROR)

            if instance.status not in no_retry_statuses:
                logger.debug(f"adding {instance.name}")
                if instance.status != TwisterStatus.NONE:
                    instance.retries += 1
                instance.status = TwisterStatus.NONE
                # Previous states should be removed from the stats
                if self.results.iteration > 1:
                    ProjectBuilder._add_instance_testcases_to_status_counts(
                        instance,
                        self.results,
                        decrement=True
                    )

                # Check if cmake package_helper script can be run in advance.
                instance.filter_stages = []
                if instance.testsuite.filter:
                    instance.filter_stages = self.get_cmake_filter_stages(
                        instance.testsuite.filter,
                        expr_parser.reserved.keys()
                    )

                if test_only and instance.run:
                    pipeline.put({"op": "run", "test": instance})
                elif instance.filter_stages and "full" not in instance.filter_stages:
                    pipeline.put({"op": "filter", "test": instance})
                else:
                    cache_file = os.path.join(instance.build_dir, "CMakeCache.txt")
                    if os.path.exists(cache_file) and self.env.options.aggressive_no_clean:
                        pipeline.put({"op": "build", "test": instance})
                    else:
                        pipeline.put({"op": "cmake", "test": instance})


    def pipeline_mgr(self, pipeline, done_queue, lock, results):
        try:
            if sys.platform == 'linux':
                with self.jobserver.get_job():
                    while True:
                        try:
                            task = pipeline.get_nowait()
                        except queue.Empty:
                            break
                        else:
                            instance = task['test']
                            pb = ProjectBuilder(instance, self.env, self.jobserver)
                            pb.duts = self.duts
                            pb.process(pipeline, done_queue, task, lock, results)
                            if self.env.options.quit_on_failure and \
                                pb.instance.status in [TwisterStatus.FAIL, TwisterStatus.ERROR]:
                                with pipeline.mutex:
                                    pipeline.queue.clear()
                                break

                    return True
            else:
                while True:
                    try:
                        task = pipeline.get_nowait()
                    except queue.Empty:
                        break
                    else:
                        instance = task['test']
                        pb = ProjectBuilder(instance, self.env, self.jobserver)
                        pb.duts = self.duts
                        pb.process(pipeline, done_queue, task, lock, results)
                        if self.env.options.quit_on_failure and \
                            pb.instance.status in [TwisterStatus.FAIL, TwisterStatus.ERROR]:
                            with pipeline.mutex:
                                pipeline.queue.clear()
                            break
                return True
        except Exception as e:
            logger.error(f"General exception: {e}")
            sys.exit(1)

    def execute(self, pipeline, done):
        lock = Lock()
        logger.info("Adding tasks to the queue...")
        self.add_tasks_to_queue(pipeline, self.options.build_only, self.options.test_only,
                                retry_build_errors=self.options.retry_build_errors)
        logger.info("Added initial list of jobs to queue")

        processes = []

        for _ in range(self.jobs):
            p = Process(target=self.pipeline_mgr, args=(pipeline, done, lock, self.results, ))
            processes.append(p)
            p.start()
        logger.debug(f"Launched {self.jobs} jobs")

        try:
            for p in processes:
                p.join()
                if p.exitcode != 0:
                    logger.error(f"Process {p.pid} failed, aborting execution")
                    for proc in processes:
                        proc.terminate()
                    sys.exit(1)
        except KeyboardInterrupt:
            logger.info("Execution interrupted")
            for p in processes:
                p.terminate()

    @staticmethod
    def get_cmake_filter_stages(filt, logic_keys):
        """Analyze filter expressions from test yaml
        and decide if dts and/or kconfig based filtering will be needed.
        """
        dts_required = False
        kconfig_required = False
        full_required = False
        filter_stages = []

        # Compress args in expressions like "function('x', 'y')"
        # so they are not split when splitting by whitespaces
        filt = filt.replace(", ", ",")
        # Remove logic words
        for k in logic_keys:
            filt = filt.replace(f"{k} ", "")
        # Remove brackets
        filt = filt.replace("(", "")
        filt = filt.replace(")", "")
        # Splite by whitespaces
        filt = filt.split()
        for expression in filt:
            if expression.startswith("dt_"):
                dts_required = True
            elif expression.startswith("CONFIG"):
                kconfig_required = True
            else:
                full_required = True

        if full_required:
            return ["full"]
        if dts_required:
            filter_stages.append("dts")
        if kconfig_required:
            filter_stages.append("kconfig")

        return filter_stages
