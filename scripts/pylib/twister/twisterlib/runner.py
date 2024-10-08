# vim: set syntax=python ts=4 :
#
# Copyright (c) 20180-2022 Intel Corporation
# Copyright 2022 NXP
# SPDX-License-Identifier: Apache-2.0

import logging
import multiprocessing
import os
import pickle
import queue
import re
import shutil
import subprocess
import sys
import time
import traceback
import yaml
from multiprocessing import Lock, Process, Value
from multiprocessing.managers import BaseManager
from typing import List
from packaging import version

from colorama import Fore
from domains import Domains
from twisterlib.cmakecache import CMakeCache
from twisterlib.environment import canonical_zephyr_base
from twisterlib.error import BuildError, ConfigurationError, StatusAttributeError
from twisterlib.statuses import TwisterStatus

import elftools
from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection

if version.parse(elftools.__version__) < version.parse('0.24'):
    sys.exit("pyelftools is out of date, need version 0.24 or later")

# Job server only works on Linux for now.
if sys.platform == 'linux':
    from twisterlib.jobserver import GNUMakeJobClient, GNUMakeJobServer, JobClient

from twisterlib.log_helper import log_command
from twisterlib.testinstance import TestInstance
from twisterlib.environment import TwisterEnv
from twisterlib.testsuite import TestSuite
from twisterlib.platform import Platform
from twisterlib.testplan import change_skip_to_error_if_integration
from twisterlib.harness import HarnessImporter, Pytest

try:
    from yaml import CSafeLoader as SafeLoader
except ImportError:
    from yaml import SafeLoader

logger = logging.getLogger('twister')
logger.setLevel(logging.DEBUG)
import expr_parser


class ExecutionCounter(object):
    def __init__(self, total=0):
        '''
        Most of the stats are at test instance level
        Except that "_cases" and "_skipped_cases" are for cases of ALL test instances

        total complete = done + skipped_filter
        total = yaml test scenarios * applicable platforms
        complete perctenage = (done + skipped_filter) / total
        pass rate = passed / (total - skipped_configs)
        '''
        # instances that go through the pipeline
        # updated by report_out()
        self._done = Value('i', 0)

        # iteration
        self._iteration = Value('i', 0)

        # instances that actually executed and passed
        # updated by report_out()
        self._passed = Value('i', 0)

        # static filter + runtime filter + build skipped
        # updated by update_counting_before_pipeline() and report_out()
        self._skipped_configs = Value('i', 0)

        # cmake filter + build skipped
        # updated by report_out()
        self._skipped_runtime = Value('i', 0)

        # staic filtered at yaml parsing time
        # updated by update_counting_before_pipeline()
        self._skipped_filter = Value('i', 0)

        # updated by update_counting_before_pipeline() and report_out()
        self._skipped_cases = Value('i', 0)

        # updated by report_out() in pipeline
        self._error = Value('i', 0)
        self._failed = Value('i', 0)

        # initialized to number of test instances
        self._total = Value('i', total)

        # updated in report_out
        self._cases = Value('i', 0)
        self.lock = Lock()

    def summary(self):
        print("--------------------------------")
        print(f"Total test suites: {self.total}") # actually test instances
        print(f"Total test cases: {self.cases}")
        print(f"Executed test cases: {self.cases - self.skipped_cases}")
        print(f"Skipped test cases: {self.skipped_cases}")
        print(f"Completed test suites: {self.done}")
        print(f"Passing test suites: {self.passed}")
        print(f"Failing test suites: {self.failed}")
        print(f"Skipped test suites: {self.skipped_configs}")
        print(f"Skipped test suites (runtime): {self.skipped_runtime}")
        print(f"Skipped test suites (filter): {self.skipped_filter}")
        print(f"Errors: {self.error}")
        print("--------------------------------")

    @property
    def cases(self):
        with self._cases.get_lock():
            return self._cases.value

    @cases.setter
    def cases(self, value):
        with self._cases.get_lock():
            self._cases.value = value

    @property
    def skipped_cases(self):
        with self._skipped_cases.get_lock():
            return self._skipped_cases.value

    @skipped_cases.setter
    def skipped_cases(self, value):
        with self._skipped_cases.get_lock():
            self._skipped_cases.value = value

    @property
    def error(self):
        with self._error.get_lock():
            return self._error.value

    @error.setter
    def error(self, value):
        with self._error.get_lock():
            self._error.value = value

    @property
    def iteration(self):
        with self._iteration.get_lock():
            return self._iteration.value

    @iteration.setter
    def iteration(self, value):
        with self._iteration.get_lock():
            self._iteration.value = value

    @property
    def done(self):
        with self._done.get_lock():
            return self._done.value

    @done.setter
    def done(self, value):
        with self._done.get_lock():
            self._done.value = value

    @property
    def passed(self):
        with self._passed.get_lock():
            return self._passed.value

    @passed.setter
    def passed(self, value):
        with self._passed.get_lock():
            self._passed.value = value

    @property
    def skipped_configs(self):
        with self._skipped_configs.get_lock():
            return self._skipped_configs.value

    @skipped_configs.setter
    def skipped_configs(self, value):
        with self._skipped_configs.get_lock():
            self._skipped_configs.value = value

    @property
    def skipped_filter(self):
        with self._skipped_filter.get_lock():
            return self._skipped_filter.value

    @skipped_filter.setter
    def skipped_filter(self, value):
        with self._skipped_filter.get_lock():
            self._skipped_filter.value = value

    @property
    def skipped_runtime(self):
        with self._skipped_runtime.get_lock():
            return self._skipped_runtime.value

    @skipped_runtime.setter
    def skipped_runtime(self, value):
        with self._skipped_runtime.get_lock():
            self._skipped_runtime.value = value

    @property
    def failed(self):
        with self._failed.get_lock():
            return self._failed.value

    @failed.setter
    def failed(self, value):
        with self._failed.get_lock():
            self._failed.value = value

    @property
    def total(self):
        with self._total.get_lock():
            return self._total.value

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

    def parse_generated(self, filter_stages=[]):
        self.defconfig = {}
        return {}

    def run_build(self, args=[]):

        logger.debug("Building %s for %s" % (self.source_dir, self.platform.name))

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
            msg = f"Finished building {self.source_dir} for {self.platform.name} in {duration:.2f} seconds"
            logger.debug(msg)

            self.instance.status = TwisterStatus.PASS
            if not self.instance.run:
                self.instance.add_missing_case_status(TwisterStatus.SKIP, "Test was built only")
            ret = {"returncode": p.returncode}

            if out:
                log_msg = out.decode(self.default_encoding)
                with open(os.path.join(self.build_dir, self.log), "a", encoding=self.default_encoding) as log:
                    log.write(log_msg)
            else:
                return None
        else:
            # A real error occurred, raise an exception
            log_msg = ""
            if out:
                log_msg = out.decode(self.default_encoding)
                with open(os.path.join(self.build_dir, self.log), "a", encoding=self.default_encoding) as log:
                    log.write(log_msg)

            if log_msg:
                overflow_found = re.findall("region `(FLASH|ROM|RAM|ICCM|DCCM|SRAM|dram\\d_\\d_seg)' overflowed by", log_msg)
                imgtool_overflow_found = re.findall(r"Error: Image size \(.*\) \+ trailer \(.*\) exceeds requested size", log_msg)
                if overflow_found and not self.options.overflow_as_errors:
                    logger.debug("Test skipped due to {} Overflow".format(overflow_found[0]))
                    self.instance.status = TwisterStatus.SKIP
                    self.instance.reason = "{} overflow".format(overflow_found[0])
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

    def run_cmake(self, args="", filter_stages=[]):

        if not self.options.disable_warnings_as_errors:
            warnings_as_errors = 'y'
            gen_edt_args = "--edtlib-Werror"
        else:
            warnings_as_errors = 'n'
            gen_edt_args = ""

        warning_command = 'CONFIG_COMPILER_WARNINGS_AS_ERRORS'
        if self.instance.sysbuild:
            warning_command = 'SB_' + warning_command

        logger.debug("Running cmake on %s for %s" % (self.source_dir, self.platform.name))
        cmake_args = [
            f'-B{self.build_dir}',
            f'-DTC_RUNID={self.instance.run_id}',
            f'-DTC_NAME={self.instance.testsuite.name}',
            f'-D{warning_command}={warnings_as_errors}',
            f'-DEXTRA_GEN_EDT_ARGS={gen_edt_args}',
            f'-G{self.env.generator}'
        ]

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
            logger.debug("Building %s using sysbuild" % (self.source_dir))
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

        cmake_opts = ['-DBOARD={}'.format(self.platform.name)]
        cmake_args.extend(cmake_opts)

        if self.instance.testsuite.required_snippets:
            cmake_opts = ['-DSNIPPET={}'.format(';'.join(self.instance.testsuite.required_snippets))]
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
            msg = f"Finished running cmake {self.source_dir} for {self.platform.name} in {duration:.2f} seconds"
            logger.debug(msg)
            ret = {
                    'returncode': p.returncode,
                    'filter': filter_results
                    }
        else:
            self.instance.status = TwisterStatus.ERROR
            self.instance.reason = "Cmake build failure"

            for tc in self.instance.testcases:
                tc.status = self.instance.status

            logger.error("Cmake build failure: %s for %s" % (self.source_dir, self.platform.name))
            ret = {"returncode": p.returncode}

        if out:
            os.makedirs(self.build_dir, exist_ok=True)
            with open(os.path.join(self.build_dir, self.log), "a", encoding=self.default_encoding) as log:
                log_msg = out.decode(self.default_encoding)
                log.write(log_msg)

        return ret


class FilterBuilder(CMake):

    def __init__(self, testsuite: TestSuite, platform: Platform, source_dir, build_dir, jobserver):
        super().__init__(testsuite, platform, source_dir, build_dir, jobserver)

        self.log = "config-twister.log"

    def parse_generated(self, filter_stages=[]):

        if self.platform.name == "unit_testing":
            return {}

        if self.instance.sysbuild and not filter_stages:
            # Load domain yaml to get default domain build directory
            domain_path = os.path.join(self.build_dir, "domains.yaml")
            domains = Domains.from_file(domain_path)
            logger.debug("Loaded sysbuild domain data from %s" % (domain_path))
            self.instance.domains = domains
            domain_build = domains.get_default_domain().build_dir
            cmake_cache_path = os.path.join(domain_build, "CMakeCache.txt")
            defconfig_path = os.path.join(domain_build, "zephyr", ".config")
            edt_pickle = os.path.join(domain_build, "zephyr", "edt.pickle")
        else:
            cmake_cache_path = os.path.join(self.build_dir, "CMakeCache.txt")
            # .config is only available after kconfig stage in cmake. If only dt based filtration is required
            # package helper call won't produce .config
            if not filter_stages or "kconfig" in filter_stages:
                defconfig_path = os.path.join(self.build_dir, "zephyr", ".config")
            # dt is compiled before kconfig, so edt_pickle is available regardless of choice of filter stages
            edt_pickle = os.path.join(self.build_dir, "zephyr", "edt.pickle")


        if not filter_stages or "kconfig" in filter_stages:
            with open(defconfig_path, "r") as fp:
                defconfig = {}
                for line in fp.readlines():
                    m = self.config_re.match(line)
                    if not m:
                        if line.strip() and not line.startswith("#"):
                            sys.stderr.write("Unrecognized line %s\n" % line)
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

        if self.instance.sysbuild and self.env.options.device_testing:
            # Verify that twister's arguments support sysbuild.
            # Twister sysbuild flashing currently only works with west, so
            # --west-flash must be passed.
            if self.env.options.west_flash is None:
                logger.warning("Sysbuild test will be skipped. " +
                    "West must be used for flashing.")
                return {os.path.join(self.platform.name, self.testsuite.name): True}

        if self.testsuite and self.testsuite.filter:
            try:
                if os.path.exists(edt_pickle):
                    with open(edt_pickle, 'rb') as f:
                        edt = pickle.load(f)
                else:
                    edt = None
                ret = expr_parser.parse(self.testsuite.filter, filter_data, edt)

            except (ValueError, SyntaxError) as se:
                sys.stderr.write(
                    "Failed processing %s\n" % self.testsuite.yamlfile)
                raise se

            if not ret:
                return {os.path.join(self.platform.name, self.testsuite.name): True}
            else:
                return {os.path.join(self.platform.name, self.testsuite.name): False}
        else:
            self.platform.filter_data = filter_data
            return filter_data


class ProjectBuilder(FilterBuilder):

    def __init__(self, instance: TestInstance, env: TwisterEnv, jobserver, **kwargs):
        super().__init__(instance.testsuite, instance.platform, instance.testsuite.source_dir, instance.build_dir, jobserver)

        self.log = "build.log"
        self.instance = instance
        self.filtered_tests = 0
        self.options = env.options
        self.env = env
        self.duts = None

    def log_info(self, filename, inline_logs, log_testcases=False):
        filename = os.path.abspath(os.path.realpath(filename))
        if inline_logs:
            logger.info("{:-^100}".format(filename))

            try:
                with open(filename) as fp:
                    data = fp.read()
            except Exception as e:
                data = "Unable to read log data (%s)\n" % (str(e))

            # Remove any coverage data from the dumped logs
            data = re.sub(
                r"GCOV_COVERAGE_DUMP_START.*GCOV_COVERAGE_DUMP_END",
                "GCOV_COVERAGE_DUMP_START\n...\nGCOV_COVERAGE_DUMP_END",
                data,
                flags=re.DOTALL,
            )
            logger.error(data)

            logger.info("{:-^100}".format(filename))

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
        h_log = "{}/handler.log".format(build_dir)
        he_log = "{}/handler_stderr.log".format(build_dir)
        b_log = "{}/build.log".format(build_dir)
        v_log = "{}/valgrind.log".format(build_dir)
        d_log = "{}/device.log".format(build_dir)
        pytest_log = "{}/twister_harness.log".format(build_dir)

        if os.path.exists(v_log) and "Valgrind" in self.instance.reason:
            self.log_info("{}".format(v_log), inline_logs)
        elif os.path.exists(pytest_log) and os.path.getsize(pytest_log) > 0:
            self.log_info("{}".format(pytest_log), inline_logs, log_testcases=True)
        elif os.path.exists(h_log) and os.path.getsize(h_log) > 0:
            self.log_info("{}".format(h_log), inline_logs)
        elif os.path.exists(he_log) and os.path.getsize(he_log) > 0:
            self.log_info("{}".format(he_log), inline_logs)
        elif os.path.exists(d_log) and os.path.getsize(d_log) > 0:
            self.log_info("{}".format(d_log), inline_logs)
        else:
            self.log_info("{}".format(b_log), inline_logs)


    def _add_to_pipeline(self, pipeline, op: str, additionals: dict={}):
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
                        logger.debug("filtering %s" % self.instance.name)
                        self.instance.status = TwisterStatus.FILTER
                        self.instance.reason = "runtime filter"
                        results.skipped_runtime += 1
                        self.instance.add_missing_case_status(TwisterStatus.SKIP)
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
                        self.instance.status = TwisterStatus.PASS
                    next_op = 'report'
                else:
                    # Here we check the runtime filter results coming from running cmake
                    if self.instance.name in ret['filter'] and ret['filter'][self.instance.name]:
                        logger.debug("filtering %s" % self.instance.name)
                        self.instance.status = TwisterStatus.FILTER
                        self.instance.reason = "runtime filter"
                        results.skipped_runtime += 1
                        self.instance.add_missing_case_status(TwisterStatus.SKIP)
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
                logger.debug("build test: %s" % self.instance.name)
                ret = self.build()
                if not ret:
                    self.instance.status = TwisterStatus.ERROR
                    self.instance.reason = "Build Failure"
                    next_op = 'report'
                else:
                    # Count skipped cases during build, for example
                    # due to ram/rom overflow.
                    if  self.instance.status == TwisterStatus.SKIP:
                        results.skipped_runtime += 1
                        self.instance.add_missing_case_status(TwisterStatus.SKIP, self.instance.reason)

                    if ret.get('returncode', 1) > 0:
                        self.instance.add_missing_case_status(TwisterStatus.BLOCK, self.instance.reason)
                        next_op = 'report'
                    else:
                        if self.instance.testsuite.harness in ['ztest', 'test']:
                            logger.debug(f"Determine test cases for test instance: {self.instance.name}")
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
                logger.debug("run test: %s" % self.instance.name)
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
                    elif self.options.runtime_artifact_cleanup == "pass" and self.instance.status == TwisterStatus.PASS:
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
                elif mode == "passed" or (mode == "all" and self.instance.reason != "Cmake build failure"):
                    self.cleanup_artifacts()
            except StatusAttributeError as sae:
                logger.error(str(sae))
                self.instance.status = TwisterStatus.ERROR
                reason = 'Incorrect status assignment'
                self.instance.reason = reason
                self.instance.add_missing_case_status(TwisterStatus.BLOCK, reason)

    def determine_testcases(self, results):
        yaml_testsuite_name = self.instance.testsuite.id
        logger.debug(f"Determine test cases for test suite: {yaml_testsuite_name}")

        elf_file = self.instance.get_elf_file()
        elf = ELFFile(open(elf_file, "rb"))

        logger.debug(f"Test instance {self.instance.name} already has {len(self.instance.testcases)} cases.")
        new_ztest_unit_test_regex = re.compile(r"z_ztest_unit_test__([^\s]+?)__([^\s]*)")
        detected_cases = []
        for section in elf.iter_sections():
            if isinstance(section, SymbolTableSection):
                for sym in section.iter_symbols():
                    # It is only meant for new ztest fx because only new ztest fx exposes test functions
                    # precisely.

                    # The 1st capture group is new ztest suite name.
                    # The 2nd capture group is new ztest unit test name.
                    matches = new_ztest_unit_test_regex.findall(sym.name)
                    if matches:
                        for m in matches:
                            # new_ztest_suite = m[0] # not used for now
                            test_func_name = m[1].replace("test_", "", 1)
                            testcase_id = f"{yaml_testsuite_name}.{test_func_name}"
                            detected_cases.append(testcase_id)

        if detected_cases:
            logger.debug(f"{', '.join(detected_cases)} in {elf_file}")
            self.instance.testcases.clear()
            self.instance.testsuite.testcases.clear()

            # When the old regex-based test case collection is fully deprecated,
            # this will be the sole place where test cases get added to the test instance.
            # Then we can further include the new_ztest_suite info in the testcase_id.

            for testcase_id in detected_cases:
                self.instance.add_testcase(name=testcase_id)
                self.instance.testsuite.add_testcase(name=testcase_id)


    def cleanup_artifacts(self, additional_keep: List[str] = []):
        logger.debug("Cleaning up {}".format(self.instance.build_dir))
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
        logger.debug("Cleaning up for Device Testing {}".format(self.instance.build_dir))

        files_to_keep = self._get_binaries()
        files_to_keep.append(os.path.join('zephyr', 'runners.yaml'))

        if self.instance.sysbuild:
            files_to_keep.append('domains.yaml')
            for domain in self.instance.domains.get_domains():
                files_to_keep += self._get_artifact_allow_list_for_domain(domain.name)

        self.cleanup_artifacts(files_to_keep)

        self._sanitize_files()

    def _get_artifact_allow_list_for_domain(self, domain: str) -> List[str]:
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

    def _get_binaries(self) -> List[str]:
        """
        Get list of binaries paths (absolute or relative to the
        self.instance.build_dir), basing on information from platform.binaries
        or runners.yaml. If they are not found take default binaries like
        "zephyr/zephyr.hex" etc.
        """
        binaries: List[str] = []

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

    def _get_binaries_from_runners(self, domain='') -> List[str]:
        """
        Get list of binaries paths (absolute or relative to the
        self.instance.build_dir) from runners.yaml file. May be used for
        multiple-domain builds by passing in one domain at a time.
        """

        runners_file_path: str = os.path.join(self.instance.build_dir,
                                              domain, 'zephyr', 'runners.yaml')
        if not os.path.exists(runners_file_path):
            return []

        with open(runners_file_path, 'r') as file:
            runners_content: dict = yaml.load(file, Loader=SafeLoader)

        if 'config' not in runners_content:
            return []

        runners_config: dict = runners_content['config']
        binary_keys: List[str] = ['elf_file', 'hex_file', 'bin_file']

        binaries: List[str] = []
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

        with open(runners_file_path, 'rt') as file:
            runners_content_text = file.read()
            runners_content_yaml: dict = yaml.load(runners_content_text, Loader=SafeLoader)

        if 'config' not in runners_content_yaml:
            return

        runners_config: dict = runners_content_yaml['config']
        binary_keys: List[str] = ['elf_file', 'hex_file', 'bin_file']

        for binary_key in binary_keys:
            binary_path = runners_config.get(binary_key)
            # sanitize only paths which exist and are absolute
            if binary_path is None or not os.path.isabs(binary_path):
                continue
            binary_path_relative = os.path.relpath(binary_path, start=runners_dir_path)
            runners_content_text = runners_content_text.replace(binary_path, binary_path_relative)

        with open(runners_file_path, 'wt') as file:
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

            with open(file_path, "rt") as file:
                data = file.read()

            # add trailing slash at the end of canonical_zephyr_base if it does not exist:
            path_to_remove = os.path.join(canonical_zephyr_base, "")
            data = data.replace(path_to_remove, "")

            with open(file_path, "wt") as file:
                file.write(data)

    def report_out(self, results):
        total_to_do = results.total
        total_tests_width = len(str(total_to_do))
        results.done += 1
        instance = self.instance
        if results.iteration == 1:
            results.cases += len(instance.testcases)

        if instance.status in [TwisterStatus.ERROR, TwisterStatus.FAIL]:
            if instance.status == TwisterStatus.ERROR:
                results.error += 1
                txt = " ERROR "
            else:
                results.failed += 1
                txt = " FAILED "
            if self.options.verbose:
                status = Fore.RED + txt + Fore.RESET + instance.reason
            else:
                logger.error(
                    "{:<25} {:<50} {}{}{}: {}".format(
                        instance.platform.name,
                        instance.testsuite.name,
                        Fore.RED,
                        txt,
                        Fore.RESET,
                        instance.reason))
            if not self.options.verbose:
                self.log_info_file(self.options.inline_logs)
        elif instance.status in [TwisterStatus.SKIP, TwisterStatus.FILTER]:
            status = Fore.YELLOW + "SKIPPED" + Fore.RESET
            results.skipped_configs += 1
            # test cases skipped at the test instance level
            results.skipped_cases += len(instance.testsuite.testcases)
        elif instance.status == TwisterStatus.PASS:
            status = Fore.GREEN + "PASSED" + Fore.RESET
            results.passed += 1
            for case in instance.testcases:
                # test cases skipped at the test case level
                if case.status == TwisterStatus.SKIP:
                    results.skipped_cases += 1
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
                        more_info += " {:.3f}s".format(htime)
                else:
                    more_info = "build"

                if ( instance.status in [TwisterStatus.ERROR, TwisterStatus.FAIL]
                     and hasattr(self.instance.handler, 'seed')
                     and self.instance.handler.seed is not None ):
                    more_info += "/seed: " + str(self.options.seed)
            logger.info("{:>{}}/{} {:<25} {:<50} {} ({})".format(
                results.done, total_tests_width, total_to_do , instance.platform.name,
                instance.testsuite.name, status, more_info))

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
                completed_perc = int((float(results.done) / total_to_do) * 100)

            sys.stdout.write("INFO    - Total complete: %s%4d/%4d%s  %2d%%  skipped: %s%4d%s, failed: %s%4d%s, error: %s%4d%s\r" % (
                Fore.GREEN,
                results.done,
                total_to_do,
                Fore.RESET,
                completed_perc,
                Fore.YELLOW if results.skipped_configs > 0 else Fore.RESET,
                results.skipped_configs,
                Fore.RESET,
                Fore.RED if results.failed > 0 else Fore.RESET,
                results.failed,
                Fore.RESET,
                Fore.RED if results.error > 0 else Fore.RESET,
                results.error,
                Fore.RESET
                )
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
            args.append("OVERLAY_CONFIG=\"%s\"" % (" ".join(overlays)))

        # Build the final argument list
        args_expanded.extend(["-D{}".format(a.replace('"', '\"')) for a in cmake_extra_args])
        args_expanded.extend(["-D{}".format(a.replace('"', '')) for a in args])

        return args_expanded

    def cmake(self, filter_stages=[]):
        args = self.cmake_assemble_args(
            self.testsuite.extra_args.copy(), # extra_args from YAML
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
            if not instance.platform.type in ["native", "qemu", "unit"]:
                generate_warning = bool(instance.platform.type == "mcu")
                size_calc = instance.calculate_sizes(from_buildlog=from_buildlog, generate_warning=generate_warning)
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

            logger.info("JOBS: %d", self.jobs)

        self.update_counting_before_pipeline()

        while True:
            self.results.iteration += 1

            if self.results.iteration > 1:
                logger.info("%d Iteration:" % (self.results.iteration))
                time.sleep(self.options.retry_interval)  # waiting for the system to settle down
                self.results.done = self.results.total - self.results.failed - self.results.error
                self.results.failed = 0
                if self.options.retry_build_errors:
                    self.results.error = 0
            else:
                self.results.done = self.results.skipped_filter

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
            if instance.status == TwisterStatus.FILTER and not instance.reason == 'runtime filter':
                self.results.skipped_filter += 1
                self.results.skipped_configs += 1
                self.results.skipped_cases += len(instance.testsuite.testcases)
                self.results.cases += len(instance.testsuite.testcases)
            elif instance.status == TwisterStatus.ERROR:
                self.results.error += 1

    def show_brief(self):
        logger.info("%d test scenarios (%d test instances) selected, "
                    "%d configurations skipped (%d by static filter, %d at runtime)." %
                    (len(self.suites), len(self.instances),
                    self.results.skipped_configs,
                    self.results.skipped_filter,
                    self.results.skipped_configs - self.results.skipped_filter))

    def add_tasks_to_queue(self, pipeline, build_only=False, test_only=False, retry_build_errors=False):
        for instance in self.instances.values():
            if build_only:
                instance.run = False

            no_retry_statuses = [TwisterStatus.PASS, TwisterStatus.SKIP, TwisterStatus.FILTER]
            if not retry_build_errors:
                no_retry_statuses.append(TwisterStatus.ERROR)

            if instance.status not in no_retry_statuses:
                logger.debug(f"adding {instance.name}")
                if instance.status != TwisterStatus.NONE:
                    instance.retries += 1
                instance.status = TwisterStatus.NONE

                # Check if cmake package_helper script can be run in advance.
                instance.filter_stages = []
                if instance.testsuite.filter:
                    instance.filter_stages = self.get_cmake_filter_stages(instance.testsuite.filter, expr_parser.reserved.keys())

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
        """ Analyze filter expressions from test yaml and decide if dts and/or kconfig based filtering will be needed."""
        dts_required = False
        kconfig_required = False
        full_required = False
        filter_stages = []

        # Compress args in expressions like "function('x', 'y')" so they are not split when splitting by whitespaces
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
