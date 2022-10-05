# vim: set syntax=python ts=4 :
#
# Copyright (c) 20180-2022 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

import os
import shutil
import re
import sys
import subprocess
import pickle
import logging
import queue
import time
import multiprocessing
import traceback
import scl
from colorama import Fore
from multiprocessing import Lock, Process, Value
from multiprocessing.managers import BaseManager
from twisterlib.cmakecache import CMakeCache
from twisterlib.environment import canonical_zephyr_base
from twisterlib.log_helper import log_command

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

        # updated in update_counting_after_pipeline()
        self._cases = Value('i', 0)
        self.lock = Lock()

    def summary(self):
        logger.debug("--------------------------------")
        logger.debug(f"Total test suites: {self.total}") # actually test instances
        logger.debug(f"Total test cases: {self.cases}")
        logger.debug(f"Executed test cases: {self.cases - self.skipped_cases}")
        logger.debug(f"Skipped test cases: {self.skipped_cases}")
        logger.debug(f"Completed test suites: {self.done}")
        logger.debug(f"Passing test suites: {self.passed}")
        logger.debug(f"Failing test suites: {self.failed}")
        logger.debug(f"Skipped test suites: {self.skipped_configs}")
        logger.debug(f"Skipped test suites (runtime): {self.skipped_runtime}")
        logger.debug(f"Skipped test suites (filter): {self.skipped_filter}")
        logger.debug(f"Errors: {self.error}")
        logger.debug("--------------------------------")

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

    def __init__(self, testsuite, platform, source_dir, build_dir):

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

    def parse_generated(self):
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

        p = subprocess.Popen(cmd, **kwargs)
        out, _ = p.communicate()

        results = {}
        if p.returncode == 0:
            msg = "Finished building %s for %s" % (self.source_dir, self.platform.name)

            self.instance.status = "passed"
            if not self.instance.run:
                self.instance.add_missing_case_status("skipped", "Test was built only")
            results = {'msg': msg, "returncode": p.returncode, "instance": self.instance}

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
                overflow_found = re.findall("region `(FLASH|ROM|RAM|ICCM|DCCM|SRAM|dram0_1_seg)' overflowed by", log_msg)
                imgtool_overflow_found = re.findall(r"Error: Image size \(.*\) \+ trailer \(.*\) exceeds requested size", log_msg)
                if overflow_found and not self.options.overflow_as_errors:
                    logger.debug("Test skipped due to {} Overflow".format(overflow_found[0]))
                    self.instance.status = "skipped"
                    self.instance.reason = "{} overflow".format(overflow_found[0])
                elif imgtool_overflow_found and not self.options.overflow_as_errors:
                    self.instance.status = "skipped"
                    self.instance.reason = "imgtool overflow"
                else:
                    self.instance.status = "error"
                    self.instance.reason = "Build failure"

            results = {
                "returncode": p.returncode,
                "instance": self.instance,
            }

        return results

    def run_cmake(self, args=""):

        if not self.options.disable_warnings_as_errors:
            ldflags = "-Wl,--fatal-warnings"
            cflags = "-Werror"
            aflags = "-Werror -Wa,--fatal-warnings"
            gen_defines_args = "--edtlib-Werror"
        else:
            ldflags = cflags = aflags = ""
            gen_defines_args = ""

        logger.debug("Running cmake on %s for %s" % (self.source_dir, self.platform.name))
        cmake_args = [
            f'-B{self.build_dir}',
            f'-DTC_RUNID={self.instance.run_id}',
            f'-DEXTRA_CFLAGS={cflags}',
            f'-DEXTRA_AFLAGS={aflags}',
            f'-DEXTRA_LDFLAGS={ldflags}',
            f'-DEXTRA_GEN_DEFINES_ARGS={gen_defines_args}',
            f'-G{self.env.generator}'
        ]

        if self.testsuite.sysbuild:
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

        cmake = shutil.which('cmake')
        cmd = [cmake] + cmake_args
        kwargs = dict()

        log_command(logger, "Calling cmake", cmd)

        if self.capture_output:
            kwargs['stdout'] = subprocess.PIPE
            # CMake sends the output of message() to stderr unless it's STATUS
            kwargs['stderr'] = subprocess.STDOUT

        if self.cwd:
            kwargs['cwd'] = self.cwd

        p = subprocess.Popen(cmd, **kwargs)
        out, _ = p.communicate()

        if p.returncode == 0:
            filter_results = self.parse_generated()
            msg = "Finished building %s for %s" % (self.source_dir, self.platform.name)
            logger.debug(msg)
            results = {'msg': msg, 'filter': filter_results}

        else:
            self.instance.status = "error"
            self.instance.reason = "Cmake build failure"

            for tc in self.instance.testcases:
                tc.status = self.instance.status

            logger.error("Cmake build failure: %s for %s" % (self.source_dir, self.platform.name))
            results = {"returncode": p.returncode}

        if out:
            with open(os.path.join(self.build_dir, self.log), "a", encoding=self.default_encoding) as log:
                log_msg = out.decode(self.default_encoding)
                log.write(log_msg)

        return results

class FilterBuilder(CMake):

    def __init__(self, testsuite, platform, source_dir, build_dir):
        super().__init__(testsuite, platform, source_dir, build_dir)

        self.log = "config-twister.log"

    def parse_generated(self):

        if self.platform.name == "unit_testing":
            return {}

        if self.testsuite.sysbuild:
            # We must parse the domains.yaml file to determine the
            # default sysbuild application
            domain_path = os.path.join(self.build_dir, "domains.yaml")
            domain_yaml = scl.yaml_load(domain_path)
            logger.debug("Loaded sysbuild domain data from %s" % (domain_path))
            default_domain = domain_yaml['default']
            for domain in domain_yaml['domains']:
                if domain['name'] == default_domain:
                    domain_build = domain['build_dir']
            cmake_cache_path = os.path.join(domain_build, "CMakeCache.txt")
            defconfig_path = os.path.join(domain_build, "zephyr", ".config")
            edt_pickle = os.path.join(domain_build, "zephyr", "edt.pickle")
        else:
            cmake_cache_path = os.path.join(self.build_dir, "CMakeCache.txt")
            defconfig_path = os.path.join(self.build_dir, "zephyr", ".config")
            edt_pickle = os.path.join(self.build_dir, "zephyr", "edt.pickle")


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
        filter_data.update(self.defconfig)
        filter_data.update(self.cmake_cache)

        if self.testsuite.sysbuild and self.env.options.device_testing:
            # Verify that twister's arguments support sysbuild.
            # Twister sysbuild flashing currently only works with west, so
            # --west-flash must be passed. Additionally, erasing the DUT
            # before each test with --west-flash=--erase will inherently not
            # work with sysbuild.
            if self.env.options.west_flash is None:
                logger.warning("Sysbuild test will be skipped. " +
                    "West must be used for flashing.")
                return {os.path.join(self.platform.name, self.testsuite.name): True}
            elif "--erase" in self.env.options.west_flash:
                logger.warning("Sysbuild test will be skipped, " +
                    "--erase is not supported with --west-flash")
                return {os.path.join(self.platform.name, self.testsuite.name): True}

        if self.testsuite and self.testsuite.filter:
            try:
                if os.path.exists(edt_pickle):
                    with open(edt_pickle, 'rb') as f:
                        edt = pickle.load(f)
                else:
                    edt = None
                res = expr_parser.parse(self.testsuite.filter, filter_data, edt)

            except (ValueError, SyntaxError) as se:
                sys.stderr.write(
                    "Failed processing %s\n" % self.testsuite.yamlfile)
                raise se

            if not res:
                return {os.path.join(self.platform.name, self.testsuite.name): True}
            else:
                return {os.path.join(self.platform.name, self.testsuite.name): False}
        else:
            self.platform.filter_data = filter_data
            return filter_data


class ProjectBuilder(FilterBuilder):

    def __init__(self, instance, env, **kwargs):
        super().__init__(instance.testsuite, instance.platform, instance.testsuite.source_dir, instance.build_dir)

        self.log = "build.log"
        self.instance = instance
        self.filtered_tests = 0
        self.options = env.options
        self.env = env
        self.duts = None

    @staticmethod
    def log_info(filename, inline_logs):
        filename = os.path.abspath(os.path.realpath(filename))
        if inline_logs:
            logger.info("{:-^100}".format(filename))

            try:
                with open(filename) as fp:
                    data = fp.read()
            except Exception as e:
                data = "Unable to read log data (%s)\n" % (str(e))

            logger.error(data)

            logger.info("{:-^100}".format(filename))
        else:
            logger.error("see: " + Fore.YELLOW + filename + Fore.RESET)

    def log_info_file(self, inline_logs):
        build_dir = self.instance.build_dir
        h_log = "{}/handler.log".format(build_dir)
        b_log = "{}/build.log".format(build_dir)
        v_log = "{}/valgrind.log".format(build_dir)
        d_log = "{}/device.log".format(build_dir)

        if os.path.exists(v_log) and "Valgrind" in self.instance.reason:
            self.log_info("{}".format(v_log), inline_logs)
        elif os.path.exists(h_log) and os.path.getsize(h_log) > 0:
            self.log_info("{}".format(h_log), inline_logs)
        elif os.path.exists(d_log) and os.path.getsize(d_log) > 0:
            self.log_info("{}".format(d_log), inline_logs)
        else:
            self.log_info("{}".format(b_log), inline_logs)


    def process(self, pipeline, done, message, lock, results):
        op = message.get('op')

        self.instance.setup_handler(self.env)

        # The build process, call cmake and build with configured generator
        if op == "cmake":
            res = self.cmake()
            if self.instance.status in ["failed", "error"]:
                pipeline.put({"op": "report", "test": self.instance})
            elif self.options.cmake_only:
                if self.instance.status is None:
                    self.instance.status = "passed"
                pipeline.put({"op": "report", "test": self.instance})
            else:
                # Here we check the runtime filter results coming from running cmake
                if self.instance.name in res['filter'] and res['filter'][self.instance.name]:
                    logger.debug("filtering %s" % self.instance.name)
                    self.instance.status = "filtered"
                    self.instance.reason = "runtime filter"
                    results.skipped_runtime += 1
                    self.instance.add_missing_case_status("skipped")
                    pipeline.put({"op": "report", "test": self.instance})
                else:
                    pipeline.put({"op": "build", "test": self.instance})

        elif op == "build":
            logger.debug("build test: %s" % self.instance.name)
            res = self.build()
            if not res:
                self.instance.status = "error"
                self.instance.reason = "Build Failure"
                pipeline.put({"op": "report", "test": self.instance})
            else:
                # Count skipped cases during build, for example
                # due to ram/rom overflow.
                if  self.instance.status == "skipped":
                    results.skipped_runtime += 1
                    self.instance.add_missing_case_status("skipped", self.instance.reason)

                if res.get('returncode', 1) > 0:
                    self.instance.add_missing_case_status("blocked", self.instance.reason)
                    pipeline.put({"op": "report", "test": self.instance})
                else:
                    logger.debug(f"Determine test cases for test instance: {self.instance.name}")
                    self.determine_testcases(results)
                    pipeline.put({"op": "gather_metrics", "test": self.instance})

        elif op == "gather_metrics":
            self.gather_metrics(self.instance)
            if self.instance.run and self.instance.handler:
                pipeline.put({"op": "run", "test": self.instance})
            else:
                pipeline.put({"op": "report", "test": self.instance})

        # Run the generated binary using one of the supported handlers
        elif op == "run":
            logger.debug("run test: %s" % self.instance.name)
            self.run()
            logger.debug(f"run status: {self.instance.name} {self.instance.status}")
            try:
                # to make it work with pickle
                self.instance.handler.thread = None
                self.instance.handler.duts = None
                pipeline.put({
                    "op": "report",
                    "test": self.instance,
                    "status": self.instance.status,
                    "reason": self.instance.reason
                    }
                )
            except RuntimeError as e:
                logger.error(f"RuntimeError: {e}")
                traceback.print_exc()

        # Report results and output progress to screen
        elif op == "report":
            with lock:
                done.put(self.instance)
                self.report_out(results)

            if self.options.runtime_artifact_cleanup and not self.options.coverage and self.instance.status == "passed":
                pipeline.put({
                    "op": "cleanup",
                    "test": self.instance
                })

        elif op == "cleanup":
            if self.options.device_testing or self.options.prep_artifacts_for_testing:
                self.cleanup_device_testing_artifacts()
            else:
                self.cleanup_artifacts()

    def determine_testcases(self, results):
        symbol_file = os.path.join(self.build_dir, "zephyr", "zephyr.symbols")
        if os.path.isfile(symbol_file):
            logger.debug(f"zephyr.symbols found: {symbol_file}")
        else:
            # No zephyr.symbols file, cannot do symbol-based test case collection
            logger.debug(f"zephyr.symbols NOT found: {symbol_file}")
            return

        yaml_testsuite_name = self.instance.testsuite.id
        logger.debug(f"Determine test cases for test suite: {yaml_testsuite_name}")

        with open(symbol_file, 'r') as fp:
            symbols = fp.read()
            logger.debug(f"Test instance {self.instance.name} already has {len(self.instance.testcases)} cases.")

            # It is only meant for new ztest fx because only new ztest fx exposes test functions
            # precisely.

            # The 1st capture group is new ztest suite name.
            # The 2nd capture group is new ztest unit test name.
            new_ztest_unit_test_regex = re.compile(r"z_ztest_unit_test__([^\s]*)__([^\s]*)")
            matches = new_ztest_unit_test_regex.findall(symbols)
            if matches:
                # this is new ztest fx
                self.instance.testcases.clear()
                self.instance.testsuite.testcases.clear()
                for m in matches:
                    # new_ztest_suite = m[0] # not used for now
                    test_func_name = m[1].replace("test_", "")
                    testcase_id = f"{yaml_testsuite_name}.{test_func_name}"
                    # When the old regex-based test case collection is fully deprecated,
                    # this will be the sole place where test cases get added to the test instance.
                    # Then we can further include the new_ztest_suite info in the testcase_id.
                    self.instance.add_testcase(name=testcase_id)
                    self.instance.testsuite.add_testcase(name=testcase_id)

    def cleanup_artifacts(self, additional_keep=[]):
        logger.debug("Cleaning up {}".format(self.instance.build_dir))
        allow = [
            'zephyr/.config',
            'handler.log',
            'build.log',
            'device.log',
            'recording.csv',
            # below ones are needed to make --test-only work as well
            'Makefile',
            'CMakeCache.txt',
            'build.ninja',
            'CMakeFiles/rules.ninja'
            ]

        allow += additional_keep

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

        sanitizelist = [
            'CMakeCache.txt',
            'zephyr/runners.yaml',
        ]
        keep = [
            'zephyr/zephyr.hex',
            'zephyr/zephyr.bin',
            'zephyr/zephyr.elf',
            ]

        keep += sanitizelist

        self.cleanup_artifacts(keep)

        # sanitize paths so files are relocatable
        for file in sanitizelist:
            file = os.path.join(self.instance.build_dir, file)

            with open(file, "rt") as fin:
                data = fin.read()
                data = data.replace(canonical_zephyr_base+"/", "")

            with open(file, "wt") as fin:
                fin.write(data)

    def report_out(self, results):
        total_to_do = results.total
        total_tests_width = len(str(total_to_do))
        results.done += 1
        instance = self.instance

        if instance.status in ["error", "failed"]:
            if instance.status == "error":
                results.error += 1
            else:
                results.failed += 1
            if self.options.verbose:
                status = Fore.RED + "FAILED " + Fore.RESET + instance.reason
            else:
                print("")
                logger.error(
                    "{:<25} {:<50} {}FAILED{}: {}".format(
                        instance.platform.name,
                        instance.testsuite.name,
                        Fore.RED,
                        Fore.RESET,
                        instance.reason))
            if not self.options.verbose:
                self.log_info_file(self.options.inline_logs)
        elif instance.status in ["skipped", "filtered"]:
            status = Fore.YELLOW + "SKIPPED" + Fore.RESET
            results.skipped_configs += 1
            # test cases skipped at the test instance level
            results.skipped_cases += len(instance.testsuite.testcases)
        elif instance.status == "passed":
            status = Fore.GREEN + "PASSED" + Fore.RESET
            results.passed += 1
            for case in instance.testcases:
                # test cases skipped at the test case level
                if case.status == 'skipped':
                    results.skipped_cases += 1
        else:
            logger.debug(f"Unknown status = {instance.status}")
            status = Fore.YELLOW + "UNKNOWN" + Fore.RESET

        if self.options.verbose:
            if self.options.cmake_only:
                more_info = "cmake"
            elif instance.status in ["skipped", "filtered"]:
                more_info = instance.reason
            else:
                if instance.handler and instance.run:
                    more_info = instance.handler.type_str
                    htime = instance.execution_time
                    if htime:
                        more_info += " {:.3f}s".format(htime)
                else:
                    more_info = "build"

                if ( instance.status in ["error", "failed", "timeout", "flash_error"]
                     and hasattr(self.instance.handler, 'seed')
                     and self.instance.handler.seed is not None ):
                    more_info += "/seed: " + str(self.options.seed)

            logger.info("{:>{}}/{} {:<25} {:<50} {} ({})".format(
                results.done + results.skipped_filter, total_tests_width, total_to_do , instance.platform.name,
                instance.testsuite.name, status, more_info))

            if instance.status in ["error", "failed", "timeout"]:
                self.log_info_file(self.options.inline_logs)
        else:
            completed_perc = 0
            if total_to_do > 0:
                completed_perc = int((float(results.done + results.skipped_filter) / total_to_do) * 100)

            sys.stdout.write("\rINFO    - Total complete: %s%4d/%4d%s  %2d%%  skipped: %s%4d%s, failed: %s%4d%s" % (
                Fore.GREEN,
                results.done + results.skipped_filter,
                total_to_do,
                Fore.RESET,
                completed_perc,
                Fore.YELLOW if results.skipped_configs > 0 else Fore.RESET,
                results.skipped_configs,
                Fore.RESET,
                Fore.RED if results.failed > 0 else Fore.RESET,
                results.failed,
                Fore.RESET
            )
                             )
        sys.stdout.flush()

    def cmake(self):

        instance = self.instance
        args = self.testsuite.extra_args[:]

        if instance.handler:
            args += instance.handler.args

        # merge overlay files into one variable
        def extract_overlays(args):
            re_overlay = re.compile('OVERLAY_CONFIG=(.*)')
            other_args = []
            overlays = []
            for arg in args:
                match = re_overlay.search(arg)
                if match:
                    overlays.append(match.group(1).strip('\'"'))
                else:
                    other_args.append(arg)

            args[:] = other_args
            return overlays

        overlays = extract_overlays(args)

        if os.path.exists(os.path.join(instance.build_dir,
                                       "twister", "testsuite_extra.conf")):
            overlays.append(os.path.join(instance.build_dir,
                                         "twister", "testsuite_extra.conf"))

        if overlays:
            args.append("OVERLAY_CONFIG=\"%s\"" % (" ".join(overlays)))

        args_expanded = ["-D{}".format(a.replace('"', '\"')) for a in self.options.extra_args]
        args_expanded = args_expanded + ["-D{}".format(a.replace('"', '')) for a in args]
        res = self.run_cmake(args_expanded)
        return res

    def build(self):
        res = self.run_build(['--build', self.build_dir])
        return res

    def run(self):

        instance = self.instance

        if instance.handler:
            if instance.handler.type_str == "device":
                instance.handler.duts = self.duts

            if(self.options.seed is not None and instance.platform.name.startswith("native_posix")):
                self.parse_generated()
                if('CONFIG_FAKE_ENTROPY_NATIVE_POSIX' in self.defconfig and
                    self.defconfig['CONFIG_FAKE_ENTROPY_NATIVE_POSIX'] == 'y'):
                    instance.handler.seed = self.options.seed

            instance.handler.handle()

        sys.stdout.flush()

    def gather_metrics(self, instance):
        if self.options.enable_size_report and not self.options.cmake_only:
            self.calc_one_elf_size(instance)
        else:
            instance.metrics["ram_size"] = 0
            instance.metrics["rom_size"] = 0
            instance.metrics["unrecognized"] = []

    @staticmethod
    def calc_one_elf_size(instance):
        if instance.status not in ["error", "failed", "skipped"]:
            if instance.platform.type != "native":
                size_calc = instance.calculate_sizes()
                instance.metrics["ram_size"] = size_calc.get_ram_size()
                instance.metrics["rom_size"] = size_calc.get_rom_size()
                instance.metrics["unrecognized"] = size_calc.unrecognized_sections()
            else:
                instance.metrics["ram_size"] = 0
                instance.metrics["rom_size"] = 0
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

    def run(self):

        retries = self.options.retry_failed + 1
        completed = 0

        BaseManager.register('LifoQueue', queue.LifoQueue)
        manager = BaseManager()
        manager.start()

        self.results = ExecutionCounter(total=len(self.instances))
        pipeline = manager.LifoQueue()
        done_queue = manager.LifoQueue()

        # Set number of jobs
        if self.options.jobs:
            self.jobs = self.options.jobs
        elif self.options.build_only:
            self.jobs = multiprocessing.cpu_count() * 2
        else:
            self.jobs = multiprocessing.cpu_count()
        logger.info("JOBS: %d" % self.jobs)

        self.update_counting_before_pipeline()

        while True:
            completed += 1

            if completed > 1:
                logger.info("%d Iteration:" % (completed))
                time.sleep(self.options.retry_interval)  # waiting for the system to settle down
                self.results.done = self.results.total - self.results.failed
                if self.options.retry_build_errors:
                    self.results.failed = 0
                    self.results.error = 0
                else:
                    self.results.failed = self.results.error

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

            retries = retries - 1
            # There are cases where failed == error (only build failures),
            # we do not try build failures.
            if retries == 0 or (self.results.failed == self.results.error and not self.options.retry_build_errors):
                break

        self.update_counting_after_pipeline()
        self.show_brief()

    def update_counting_before_pipeline(self):
        '''
        Updating counting before pipeline is necessary because statically filterd
        test instance never enter the pipeline. While some pipeline output needs
        the static filter stats. So need to prepare them before pipline starts.
        '''
        for instance in self.instances.values():
            if instance.status == 'filtered' and not instance.reason == 'runtime filter':
                self.results.skipped_filter += 1
                self.results.skipped_configs += 1
                self.results.skipped_cases += len(instance.testsuite.testcases)

    def update_counting_after_pipeline(self):
        '''
        Updating counting after pipeline is necessary because the number of test cases
        of a test instance will be refined based on zephyr.symbols as it goes through the
        pipeline. While the testsuite.testcases is obtained by scanning the source file.
        The instance.testcases is more accurate and can only be obtained after pipeline finishes.
        '''
        for instance in self.instances.values():
            self.results.cases += len(instance.testcases)

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

            no_retry_statuses = ['passed', 'skipped', 'filtered']
            if not retry_build_errors:
                no_retry_statuses.append("error")

            if instance.status not in no_retry_statuses:
                logger.debug(f"adding {instance.name}")
                instance.status = None
                if test_only and instance.run:
                    pipeline.put({"op": "run", "test": instance})
                else:
                    pipeline.put({"op": "cmake", "test": instance})

    def pipeline_mgr(self, pipeline, done_queue, lock, results):
        while True:
            try:
                task = pipeline.get_nowait()
            except queue.Empty:
                break
            else:
                instance = task['test']
                pb = ProjectBuilder(instance, self.env)
                pb.duts = self.duts
                pb.process(pipeline, done_queue, task, lock, results)

        return True

    def execute(self, pipeline, done):
        lock = Lock()
        logger.info("Adding tasks to the queue...")
        self.add_tasks_to_queue(pipeline, self.options.build_only, self.options.test_only,
                                retry_build_errors=self.options.retry_build_errors)
        logger.info("Added initial list of jobs to queue")

        processes = []
        for job in range(self.jobs):
            logger.debug(f"Launch process {job}")
            p = Process(target=self.pipeline_mgr, args=(pipeline, done, lock, self.results, ))
            processes.append(p)
            p.start()

        try:
            for p in processes:
                p.join()
        except KeyboardInterrupt:
            logger.info("Execution interrupted")
            for p in processes:
                p.terminate()
