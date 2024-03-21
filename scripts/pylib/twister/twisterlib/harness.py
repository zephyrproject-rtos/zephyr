# SPDX-License-Identifier: Apache-2.0
from __future__ import annotations
from asyncio.log import logger
import platform
import re
import os
import sys
import subprocess
import shlex
from collections import OrderedDict
import xml.etree.ElementTree as ET
import logging
import threading
import time
import shutil

from twisterlib.error import ConfigurationError
from twisterlib.environment import ZEPHYR_BASE, PYTEST_PLUGIN_INSTALLED
from twisterlib.handlers import Handler, terminate_process, SUPPORTED_SIMS_IN_PYTEST
from twisterlib.testinstance import TestInstance


logger = logging.getLogger('twister')
logger.setLevel(logging.DEBUG)

_WINDOWS = platform.system() == 'Windows'


result_re = re.compile(r".*(PASS|FAIL|SKIP) - (test_)?(\S*) in (\d*[.,]?\d*) seconds")
class Harness:
    GCOV_START = "GCOV_COVERAGE_DUMP_START"
    GCOV_END = "GCOV_COVERAGE_DUMP_END"
    FAULT = "ZEPHYR FATAL ERROR"
    RUN_PASSED = "PROJECT EXECUTION SUCCESSFUL"
    RUN_FAILED = "PROJECT EXECUTION FAILED"
    run_id_pattern = r"RunID: (?P<run_id>.*)"


    ztest_to_status = {
        'PASS': 'passed',
        'SKIP': 'skipped',
        'BLOCK': 'blocked',
        'FAIL': 'failed'
        }

    def __init__(self):
        self.state = None
        self.type = None
        self.regex = []
        self.matches = OrderedDict()
        self.ordered = True
        self.repeat = 1
        self.id = None
        self.fail_on_fault = True
        self.fault = False
        self.capture_coverage = False
        self.next_pattern = 0
        self.record = None
        self.record_pattern = None
        self.recording = []
        self.ztest = False
        self.detected_suite_names = []
        self.run_id = None
        self.matched_run_id = False
        self.run_id_exists = False
        self.instance: TestInstance | None = None
        self.testcase_output = ""
        self._match = False

    def configure(self, instance):
        self.instance = instance
        config = instance.testsuite.harness_config
        self.id = instance.testsuite.id
        self.run_id = instance.run_id
        if instance.testsuite.ignore_faults:
            self.fail_on_fault = False

        if config:
            self.type = config.get('type', None)
            self.regex = config.get('regex', [])
            self.repeat = config.get('repeat', 1)
            self.ordered = config.get('ordered', True)
            self.record = config.get('record', {})
            if self.record:
                self.record_pattern = re.compile(self.record.get("regex", ""))

    def build(self):
        pass

    def get_testcase_name(self):
        """
        Get current TestCase name.
        """
        return self.id


    def process_test(self, line):

        runid_match = re.search(self.run_id_pattern, line)
        if runid_match:
            run_id = runid_match.group("run_id")
            self.run_id_exists = True
            if run_id == str(self.run_id):
                self.matched_run_id = True

        if self.RUN_PASSED in line:
            if self.fault:
                self.state = "failed"
            else:
                self.state = "passed"

        if self.RUN_FAILED in line:
            self.state = "failed"

        if self.fail_on_fault:
            if self.FAULT == line:
                self.fault = True

        if self.GCOV_START in line:
            self.capture_coverage = True
        elif self.GCOV_END in line:
            self.capture_coverage = False

class Robot(Harness):

    is_robot_test = True

    def configure(self, instance):
        super(Robot, self).configure(instance)
        self.instance = instance

        config = instance.testsuite.harness_config
        if config:
            self.path = config.get('robot_test_path', None)

    def handle(self, line):
        ''' Test cases that make use of this harness care about results given
            by Robot Framework which is called in run_robot_test(), so works of this
            handle is trying to give a PASS or FAIL to avoid timeout, nothing
            is writen into handler.log
        '''
        self.instance.state = "passed"
        tc = self.instance.get_case_or_create(self.id)
        tc.status = "passed"

    def run_robot_test(self, command, handler):

        start_time = time.time()
        env = os.environ.copy()
        env["ROBOT_FILES"] = self.path

        with subprocess.Popen(command, stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT, cwd=self.instance.build_dir, env=env) as cmake_proc:
            out, _ = cmake_proc.communicate()

            self.instance.execution_time = time.time() - start_time

            if cmake_proc.returncode == 0:
                self.instance.status = "passed"
                # all tests in one Robot file are treated as a single test case,
                # so its status should be set accordingly to the instance status
                # please note that there should be only one testcase in testcases list
                self.instance.testcases[0].status = "passed"
            else:
                logger.error("Robot test failure: %s for %s" %
                             (handler.sourcedir, self.instance.platform.name))
                self.instance.status = "failed"
                self.instance.testcases[0].status = "failed"

            if out:
                with open(os.path.join(self.instance.build_dir, handler.log), "wt") as log:
                    log_msg = out.decode(sys.getdefaultencoding())
                    log.write(log_msg)

class Console(Harness):

    def get_testcase_name(self):
        '''
        Get current TestCase name.

        Console Harness id has only TestSuite id without TestCase name suffix.
        Only the first TestCase name might be taken if available when a Ztest with
        a single test case is configured to use this harness type for simplified
        output parsing instead of the Ztest harness as Ztest suite should do.
        '''
        if self.instance and len(self.instance.testcases) == 1:
            return self.instance.testcases[0].name
        return super(Console, self).get_testcase_name()

    def configure(self, instance):
        super(Console, self).configure(instance)
        if self.regex is None or len(self.regex) == 0:
            self.state = "failed"
            tc = self.instance.set_case_status_by_name(
                self.get_testcase_name(),
                "failed",
                f"HARNESS:{self.__class__.__name__}:no regex patterns configured."
            )
            raise ConfigurationError(self.instance.name, tc.reason)
        if self.type == "one_line":
            self.pattern = re.compile(self.regex[0])
            self.patterns_expected = 1
        elif self.type == "multi_line":
            self.patterns = []
            for r in self.regex:
                self.patterns.append(re.compile(r))
            self.patterns_expected = len(self.patterns)
        else:
            self.state = "failed"
            tc = self.instance.set_case_status_by_name(
                self.get_testcase_name(),
                "failed",
                f"HARNESS:{self.__class__.__name__}:incorrect type={self.type}"
            )
            raise ConfigurationError(self.instance.name, tc.reason)
        #

    def handle(self, line):
        if self.type == "one_line":
            if self.pattern.search(line):
                logger.debug(f"HARNESS:{self.__class__.__name__}:EXPECTED:"
                             f"'{self.pattern.pattern}'")
                self.next_pattern += 1
                self.state = "passed"
        elif self.type == "multi_line" and self.ordered:
            if (self.next_pattern < len(self.patterns) and
                self.patterns[self.next_pattern].search(line)):
                logger.debug(f"HARNESS:{self.__class__.__name__}:EXPECTED("
                             f"{self.next_pattern + 1}/{self.patterns_expected}):"
                             f"'{self.patterns[self.next_pattern].pattern}'")
                self.next_pattern += 1
                if self.next_pattern >= len(self.patterns):
                    self.state = "passed"
        elif self.type == "multi_line" and not self.ordered:
            for i, pattern in enumerate(self.patterns):
                r = self.regex[i]
                if pattern.search(line) and not r in self.matches:
                    self.matches[r] = line
                    logger.debug(f"HARNESS:{self.__class__.__name__}:EXPECTED("
                                 f"{len(self.matches)}/{self.patterns_expected}):"
                                 f"'{pattern.pattern}'")
            if len(self.matches) == len(self.regex):
                self.state = "passed"
        else:
            logger.error("Unknown harness_config type")

        if self.fail_on_fault:
            if self.FAULT in line:
                self.fault = True

        if self.GCOV_START in line:
            self.capture_coverage = True
        elif self.GCOV_END in line:
            self.capture_coverage = False

        if self.record_pattern:
            match = self.record_pattern.search(line)
            if match:
                self.recording.append({ k:v.strip() for k,v in match.groupdict(default="").items() })

        self.process_test(line)
        # Reset the resulting test state to 'failed' when not all of the patterns were
        # found in the output, but just ztest's 'PROJECT EXECUTION SUCCESSFUL'.
        # It might happen because of the pattern sequence diverged from the
        # test code, the test platform has console issues, or even some other
        # test image was executed.
        # TODO: Introduce explicit match policy type to reject
        # unexpected console output, allow missing patterns, deny duplicates.
        if self.state == "passed" and self.ordered and self.next_pattern < self.patterns_expected:
            logger.error(f"HARNESS:{self.__class__.__name__}: failed with"
                         f" {self.next_pattern} of {self.patterns_expected}"
                         f" expected ordered patterns.")
            self.state = "failed"
        if self.state == "passed" and not self.ordered and len(self.matches) < self.patterns_expected:
            logger.error(f"HARNESS:{self.__class__.__name__}: failed with"
                         f" {len(self.matches)} of {self.patterns_expected}"
                         f" expected unordered patterns.")
            self.state = "failed"

        tc = self.instance.get_case_or_create(self.get_testcase_name())
        if self.state == "passed":
            tc.status = "passed"
        else:
            tc.status = "failed"


class PytestHarnessException(Exception):
    """General exception for pytest."""


class Pytest(Harness):

    def configure(self, instance: TestInstance):
        super(Pytest, self).configure(instance)
        self.running_dir = instance.build_dir
        self.source_dir = instance.testsuite.source_dir
        self.report_file = os.path.join(self.running_dir, 'report.xml')
        self.pytest_log_file_path = os.path.join(self.running_dir, 'twister_harness.log')
        self.reserved_serial = None

    def pytest_run(self, timeout):
        try:
            cmd = self.generate_command()
            self.run_command(cmd, timeout)
        except PytestHarnessException as pytest_exception:
            logger.error(str(pytest_exception))
            self.state = 'failed'
            self.instance.reason = str(pytest_exception)
        finally:
            if self.reserved_serial:
                self.instance.handler.make_device_available(self.reserved_serial)
        self._update_test_status()

    def generate_command(self):
        config = self.instance.testsuite.harness_config
        handler: Handler = self.instance.handler
        pytest_root = config.get('pytest_root', ['pytest']) if config else ['pytest']
        pytest_args_yaml = config.get('pytest_args', []) if config else []
        pytest_dut_scope = config.get('pytest_dut_scope', None) if config else None
        command = [
            'pytest',
            '--twister-harness',
            '-s', '-v',
            f'--build-dir={self.running_dir}',
            f'--junit-xml={self.report_file}',
            '--log-file-level=DEBUG',
            '--log-file-format=%(asctime)s.%(msecs)d:%(levelname)s:%(name)s: %(message)s',
            f'--log-file={self.pytest_log_file_path}'
        ]
        command.extend([os.path.normpath(os.path.join(
            self.source_dir, os.path.expanduser(os.path.expandvars(src)))) for src in pytest_root])

        if pytest_dut_scope:
            command.append(f'--dut-scope={pytest_dut_scope}')

        if handler.options.verbose > 1:
            command.extend([
                '--log-cli-level=DEBUG',
                '--log-cli-format=%(levelname)s: %(message)s'
            ])

        if handler.type_str == 'device':
            command.extend(
                self._generate_parameters_for_hardware(handler)
            )
        elif handler.type_str in SUPPORTED_SIMS_IN_PYTEST:
            command.append(f'--device-type={handler.type_str}')
        elif handler.type_str == 'build':
            command.append('--device-type=custom')
        else:
            raise PytestHarnessException(f'Handling of handler {handler.type_str} not implemented yet')

        if handler.options.pytest_args:
            command.extend(handler.options.pytest_args)
            if pytest_args_yaml:
                logger.warning(f'The pytest_args ({handler.options.pytest_args}) specified '
                               'in the command line will override the pytest_args defined '
                               f'in the YAML file {pytest_args_yaml}')
        else:
            command.extend(pytest_args_yaml)

        return command

    def _generate_parameters_for_hardware(self, handler: Handler):
        command = ['--device-type=hardware']
        hardware = handler.get_hardware()
        if not hardware:
            raise PytestHarnessException('Hardware is not available')
        # update the instance with the device id to have it in the summary report
        self.instance.dut = hardware.id

        self.reserved_serial = hardware.serial_pty or hardware.serial
        if hardware.serial_pty:
            command.append(f'--device-serial-pty={hardware.serial_pty}')
        else:
            command.extend([
                f'--device-serial={hardware.serial}',
                f'--device-serial-baud={hardware.baud}'
            ])

        options = handler.options
        if runner := hardware.runner or options.west_runner:
            command.append(f'--runner={runner}')

        if options.west_flash and options.west_flash != []:
            command.append(f'--west-flash-extra-args={options.west_flash}')

        if board_id := hardware.probe_id or hardware.id:
            command.append(f'--device-id={board_id}')

        if hardware.product:
            command.append(f'--device-product={hardware.product}')

        if hardware.pre_script:
            command.append(f'--pre-script={hardware.pre_script}')

        if hardware.post_flash_script:
            command.append(f'--post-flash-script={hardware.post_flash_script}')

        if hardware.post_script:
            command.append(f'--post-script={hardware.post_script}')

        return command

    def run_command(self, cmd, timeout):
        cmd, env = self._update_command_with_env_dependencies(cmd)
        with subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            env=env
        ) as proc:
            try:
                reader_t = threading.Thread(target=self._output_reader, args=(proc,), daemon=True)
                reader_t.start()
                reader_t.join(timeout)
                if reader_t.is_alive():
                    terminate_process(proc)
                    logger.warning('Timeout has occurred. Can be extended in testspec file. '
                                   f'Currently set to {timeout} seconds.')
                    self.instance.reason = 'Pytest timeout'
                    self.state = 'failed'
                proc.wait(timeout)
            except subprocess.TimeoutExpired:
                self.state = 'failed'
                proc.kill()

    @staticmethod
    def _update_command_with_env_dependencies(cmd):
        '''
        If python plugin wasn't installed by pip, then try to indicate it to
        pytest by update PYTHONPATH and append -p argument to pytest command.
        '''
        env = os.environ.copy()
        if not PYTEST_PLUGIN_INSTALLED:
            cmd.extend(['-p', 'twister_harness.plugin'])
            pytest_plugin_path = os.path.join(ZEPHYR_BASE, 'scripts', 'pylib', 'pytest-twister-harness', 'src')
            env['PYTHONPATH'] = pytest_plugin_path + os.pathsep + env.get('PYTHONPATH', '')
            if _WINDOWS:
                cmd_append_python_path = f'set PYTHONPATH={pytest_plugin_path};%PYTHONPATH% && '
            else:
                cmd_append_python_path = f'export PYTHONPATH={pytest_plugin_path}:${{PYTHONPATH}} && '
        else:
            cmd_append_python_path = ''
        cmd_to_print = cmd_append_python_path + shlex.join(cmd)
        logger.debug('Running pytest command: %s', cmd_to_print)

        return cmd, env

    @staticmethod
    def _output_reader(proc):
        while proc.stdout.readable() and proc.poll() is None:
            line = proc.stdout.readline().decode().strip()
            if not line:
                continue
            logger.debug('PYTEST: %s', line)
        proc.communicate()

    def _update_test_status(self):
        if not self.state:
            self.instance.testcases = []
            try:
                self._parse_report_file(self.report_file)
            except Exception as e:
                logger.error(f'Error when parsing file {self.report_file}: {e}')
                self.state = 'failed'
            finally:
                if not self.instance.testcases:
                    self.instance.init_cases()

        self.instance.status = self.state or 'failed'
        if self.instance.status in ['error', 'failed']:
            self.instance.reason = self.instance.reason or 'Pytest failed'
            self.instance.add_missing_case_status('blocked', self.instance.reason)

    def _parse_report_file(self, report):
        tree = ET.parse(report)
        root = tree.getroot()
        if elem_ts := root.find('testsuite'):
            if elem_ts.get('failures') != '0':
                self.state = 'failed'
                self.instance.reason = f"{elem_ts.get('failures')}/{elem_ts.get('tests')} pytest scenario(s) failed"
            elif elem_ts.get('errors') != '0':
                self.state = 'error'
                self.instance.reason = 'Error during pytest execution'
            elif elem_ts.get('skipped') == elem_ts.get('tests'):
                self.state = 'skipped'
            else:
                self.state = 'passed'
            self.instance.execution_time = float(elem_ts.get('time'))

            for elem_tc in elem_ts.findall('testcase'):
                tc = self.instance.add_testcase(f"{self.id}.{elem_tc.get('name')}")
                tc.duration = float(elem_tc.get('time'))
                elem = elem_tc.find('*')
                if elem is None:
                    tc.status = 'passed'
                else:
                    if elem.tag == 'skipped':
                        tc.status = 'skipped'
                    elif elem.tag == 'failure':
                        tc.status = 'failed'
                    else:
                        tc.status = 'error'
                    tc.reason = elem.get('message')
                    tc.output = elem.text
        else:
            self.state = 'skipped'
            self.instance.reason = 'No tests collected'


class Gtest(Harness):
    ANSI_ESCAPE = re.compile(r'\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])')
    TEST_START_PATTERN = r".*\[ RUN      \] (?P<suite_name>.*)\.(?P<test_name>.*)$"
    TEST_PASS_PATTERN = r".*\[       OK \] (?P<suite_name>.*)\.(?P<test_name>.*)$"
    TEST_SKIP_PATTERN = r".*\[ DISABLED \] (?P<suite_name>.*)\.(?P<test_name>.*)$"
    TEST_FAIL_PATTERN = r".*\[  FAILED  \] (?P<suite_name>.*)\.(?P<test_name>.*)$"
    FINISHED_PATTERN = r".*\[==========\] Done running all tests\.$"

    def __init__(self):
        super().__init__()
        self.tc = None
        self.has_failures = False

    def handle(self, line):
        # Strip the ANSI characters, they mess up the patterns
        non_ansi_line = self.ANSI_ESCAPE.sub('', line)

        if self.state:
            return

        # Check if we started running a new test
        test_start_match = re.search(self.TEST_START_PATTERN, non_ansi_line)
        if test_start_match:
            # Add the suite name
            suite_name = test_start_match.group("suite_name")
            if suite_name not in self.detected_suite_names:
                self.detected_suite_names.append(suite_name)

            # Generate the internal name of the test
            name = "{}.{}.{}".format(self.id, suite_name, test_start_match.group("test_name"))

            # Assert that we don't already have a running test
            assert (
                self.tc is None
            ), "gTest error, {} didn't finish".format(self.tc)

            # Check that the instance doesn't exist yet (prevents re-running)
            tc = self.instance.get_case_by_name(name)
            assert tc is None, "gTest error, {} running twice".format(tc)

            # Create the test instance and set the context
            tc = self.instance.get_case_or_create(name)
            self.tc = tc
            self.tc.status = "started"
            self.testcase_output += line + "\n"
            self._match = True

        # Check if the test run finished
        finished_match = re.search(self.FINISHED_PATTERN, non_ansi_line)
        if finished_match:
            tc = self.instance.get_case_or_create(self.id)
            if self.has_failures or self.tc is not None:
                self.state = "failed"
                tc.status = "failed"
            else:
                self.state = "passed"
                tc.status = "passed"
            return

        # Check if the individual test finished
        state, name = self._check_result(non_ansi_line)
        if state is None or name is None:
            # Nothing finished, keep processing lines
            return

        # Get the matching test and make sure it's the same as the current context
        tc = self.instance.get_case_by_name(name)
        assert (
            tc is not None and tc == self.tc
        ), "gTest error, mismatched tests. Expected {} but got {}".format(self.tc, tc)

        # Test finished, clear the context
        self.tc = None

        # Update the status of the test
        tc.status = state
        if tc.status == "failed":
            self.has_failures = True
            tc.output = self.testcase_output
        self.testcase_output = ""
        self._match = False

    def _check_result(self, line):
        test_pass_match = re.search(self.TEST_PASS_PATTERN, line)
        if test_pass_match:
            return "passed", "{}.{}.{}".format(self.id, test_pass_match.group("suite_name"), test_pass_match.group("test_name"))
        test_skip_match = re.search(self.TEST_SKIP_PATTERN, line)
        if test_skip_match:
            return "skipped", "{}.{}.{}".format(self.id, test_skip_match.group("suite_name"), test_skip_match.group("test_name"))
        test_fail_match = re.search(self.TEST_FAIL_PATTERN, line)
        if test_fail_match:
            return "failed", "{}.{}.{}".format(self.id, test_fail_match.group("suite_name"), test_fail_match.group("test_name"))
        return None, None


class Test(Harness):
    RUN_PASSED = "PROJECT EXECUTION SUCCESSFUL"
    RUN_FAILED = "PROJECT EXECUTION FAILED"
    test_suite_start_pattern = r"Running TESTSUITE (?P<suite_name>.*)"
    ZTEST_START_PATTERN = r"START - (test_)?(.*)"

    def handle(self, line):
        test_suite_match = re.search(self.test_suite_start_pattern, line)
        if test_suite_match:
            suite_name = test_suite_match.group("suite_name")
            self.detected_suite_names.append(suite_name)

        testcase_match = re.search(self.ZTEST_START_PATTERN, line)
        if testcase_match:
            name = "{}.{}".format(self.id, testcase_match.group(2))
            tc = self.instance.get_case_or_create(name)
            # Mark the test as started, if something happens here, it is mostly
            # due to this tests, for example timeout. This should in this case
            # be marked as failed and not blocked (not run).
            tc.status = "started"

        if testcase_match or self._match:
            self.testcase_output += line + "\n"
            self._match = True

        result_match = result_re.match(line)
        # some testcases are skipped based on predicates and do not show up
        # during test execution, however they are listed in the summary. Parse
        # the summary for status and use that status instead.

        summary_re = re.compile(r"- (PASS|FAIL|SKIP) - \[([^\.]*).(test_)?(\S*)\] duration = (\d*[.,]?\d*) seconds")
        summary_match = summary_re.match(line)

        if result_match:
            matched_status = result_match.group(1)
            name = "{}.{}".format(self.id, result_match.group(3))
            tc = self.instance.get_case_or_create(name)
            tc.status = self.ztest_to_status[matched_status]
            if tc.status == "skipped":
                tc.reason = "ztest skip"
            tc.duration = float(result_match.group(4))
            if tc.status == "failed":
                tc.output = self.testcase_output
            self.testcase_output = ""
            self._match = False
            self.ztest = True
        elif summary_match:
            matched_status = summary_match.group(1)
            self.detected_suite_names.append(summary_match.group(2))
            name = "{}.{}".format(self.id, summary_match.group(4))
            tc = self.instance.get_case_or_create(name)
            tc.status = self.ztest_to_status[matched_status]
            if tc.status == "skipped":
                tc.reason = "ztest skip"
            tc.duration = float(summary_match.group(5))
            if tc.status == "failed":
                tc.output = self.testcase_output
            self.testcase_output = ""
            self._match = False
            self.ztest = True

        self.process_test(line)

        if not self.ztest and self.state:
            logger.debug(f"not a ztest and no state for  {self.id}")
            tc = self.instance.get_case_or_create(self.id)
            if self.state == "passed":
                tc.status = "passed"
            else:
                tc.status = "failed"


class Ztest(Test):
    pass


class Bsim(Harness):

    def build(self):
        """
        Copying the application executable to BabbleSim's bin directory enables
        running multidevice bsim tests after twister has built them.
        """

        if self.instance is None:
            return

        original_exe_path: str = os.path.join(self.instance.build_dir, 'zephyr', 'zephyr.exe')
        if not os.path.exists(original_exe_path):
            logger.warning('Cannot copy bsim exe - cannot find original executable.')
            return

        bsim_out_path: str = os.getenv('BSIM_OUT_PATH', '')
        if not bsim_out_path:
            logger.warning('Cannot copy bsim exe - BSIM_OUT_PATH not provided.')
            return

        new_exe_name: str = self.instance.testsuite.harness_config.get('bsim_exe_name', '')
        if new_exe_name:
            new_exe_name = f'bs_{self.instance.platform.name}_{new_exe_name}'
        else:
            new_exe_name = self.instance.name
            new_exe_name = new_exe_name.replace(os.path.sep, '_').replace('.', '_')
            new_exe_name = f'bs_{new_exe_name}'

        new_exe_path: str = os.path.join(bsim_out_path, 'bin', new_exe_name)
        logger.debug(f'Copying executable from {original_exe_path} to {new_exe_path}')
        shutil.copy(original_exe_path, new_exe_path)


class HarnessImporter:

    @staticmethod
    def get_harness(harness_name):
        thismodule = sys.modules[__name__]
        try:
            if harness_name:
                harness_class = getattr(thismodule, harness_name)
            else:
                harness_class = getattr(thismodule, 'Test')
            return harness_class()
        except AttributeError as e:
            logger.debug(f"harness {harness_name} not implemented: {e}")
            return None
