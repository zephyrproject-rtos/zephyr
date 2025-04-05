# SPDX-License-Identifier: Apache-2.0
from __future__ import annotations

import json
import logging
import os
import platform
import re
import shlex
import shutil
import subprocess
import sys
import threading
import time
import xml.etree.ElementTree as ET
from collections import OrderedDict
from enum import Enum

import junitparser.junitparser as junit
import yaml
from pytest import ExitCode
from twisterlib.constants import SUPPORTED_SIMS_IN_PYTEST
from twisterlib.environment import PYTEST_PLUGIN_INSTALLED, ZEPHYR_BASE
from twisterlib.error import ConfigurationError, StatusAttributeError
from twisterlib.handlers import Handler, terminate_process
from twisterlib.reports import ReportStatus
from twisterlib.statuses import TwisterStatus
from twisterlib.testinstance import TestInstance

logger = logging.getLogger('twister')

_WINDOWS = platform.system() == 'Windows'


class Harness:
    GCOV_START = "GCOV_COVERAGE_DUMP_START"
    GCOV_END = "GCOV_COVERAGE_DUMP_END"
    FAULT = "ZEPHYR FATAL ERROR"
    RUN_PASSED = "PROJECT EXECUTION SUCCESSFUL"
    RUN_FAILED = "PROJECT EXECUTION FAILED"
    run_id_pattern = r"RunID: (?P<run_id>.*)"

    def __init__(self):
        self._status = TwisterStatus.NONE
        self.reason = None
        self.type = None
        self.regex = []
        self.matches = OrderedDict()
        self.ordered = True
        self.id = None
        self.fail_on_fault = True
        self.fault = False
        self.capture_coverage = False
        self.next_pattern = 0
        self.record = None
        self.record_patterns = []
        self.record_merge = False
        self.record_as_json = None
        self.recording = []
        self.ztest = False
        self.detected_suite_names = []
        self.run_id = None
        self.started_suites = {}
        self.started_cases = {}
        self.matched_run_id = False
        self.run_id_exists = False
        self.instance: TestInstance | None = None
        self.testcase_output = ""
        self._match = False


    @property
    def trace(self) -> bool:
        return self.instance.handler.options.verbose > 2

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

    def configure(self, instance):
        self.instance = instance
        config = instance.testsuite.harness_config
        self.id = instance.testsuite.id
        self.run_id = instance.run_id
        self.expect_reboot = getattr(instance.testsuite, 'expect_reboot', False)
        if instance.testsuite.ignore_faults:
            self.fail_on_fault = False

        if config:
            self.type = config.get('type', None)
            self.regex = config.get('regex', [])
            self.ordered = config.get('ordered', True)
            self.record = config.get('record', {})
            if self.record:
                self.record_patterns = [re.compile(p) for p in self.record.get("regex", [])]
                self.record_merge = self.record.get("merge", False)
                self.record_as_json = self.record.get("as_json")

    def build(self):
        pass

    def get_testcase_name(self):
        """
        Get current TestCase name.
        """
        return self.id

    def translate_record(self, record: dict) -> dict:
        if self.record_as_json:
            for k in self.record_as_json:
                if k not in record:
                    continue
                try:
                    record[k] = json.loads(record[k]) if record[k] else {}
                except json.JSONDecodeError as parse_error:
                    logger.warning(f"HARNESS:{self.__class__.__name__}: recording JSON failed:"
                                   f" {parse_error} for '{k}':'{record[k]}'")
                    # Don't set the Harness state to failed for recordings.
                    record[k] = { 'ERROR': { 'msg': str(parse_error), 'doc': record[k] } }
        return record

    def parse_record(self, line) -> int:
        match_cnt = 0
        for record_pattern in self.record_patterns:
            match = record_pattern.search(line)
            if match:
                match_cnt += 1
                rec = self.translate_record(
                    { k:v.strip() for k,v in match.groupdict(default="").items() }
                )
                if self.record_merge and len(self.recording) > 0:
                    for k,v in rec.items():
                        if k in self.recording[0]:
                            if isinstance(self.recording[0][k], list):
                                self.recording[0][k].append(v)
                            else:
                                self.recording[0][k] = [self.recording[0][k], v]
                        else:
                            self.recording[0][k] = v
                else:
                    self.recording.append(rec)
        return match_cnt

    def process_test(self, line):

        self.parse_record(line)

        runid_match = re.search(self.run_id_pattern, line)
        if runid_match:
            run_id = runid_match.group("run_id")
            self.run_id_exists = True
            if run_id == str(self.run_id):
                self.matched_run_id = True

        if self.RUN_PASSED in line:
            if self.fault:
                self.status = TwisterStatus.FAIL
                self.reason = "Fault detected while running test"
            else:
                self.status = TwisterStatus.PASS

        if self.RUN_FAILED in line:
            self.status = TwisterStatus.FAIL
            self.reason = "Testsuite failed"

        if self.fail_on_fault and line == self.FAULT:
            self.fault = True

        if self.GCOV_START in line:
            self.capture_coverage = True
        elif self.GCOV_END in line:
            self.capture_coverage = False

class Robot(Harness):

    is_robot_test = True

    def configure(self, instance):
        super().configure(instance)
        self.instance = instance

        config = instance.testsuite.harness_config
        if config:
            self.path = config.get('robot_testsuite', None)
            self.option = config.get('robot_option', None)

    def handle(self, line):
        ''' Test cases that make use of this harness care about results given
            by Robot Framework which is called in run_robot_test(), so works of this
            handle is trying to give a PASS or FAIL to avoid timeout, nothing
            is written into handler.log
        '''
        self.instance.status = TwisterStatus.PASS
        tc = self.instance.get_case_or_create(self.id)
        tc.status = TwisterStatus.PASS

    def run_robot_test(self, command, handler):
        start_time = time.time()
        env = os.environ.copy()

        if self.option:
            if isinstance(self.option, list):
                for option in self.option:
                    for v in str(option).split():
                        command.append(f'{v}')
            else:
                for v in str(self.option).split():
                    command.append(f'{v}')

        if self.path is None:
            raise PytestHarnessException('The parameter robot_testsuite is mandatory')

        if isinstance(self.path, list):
            for suite in self.path:
                command.append(os.path.join(handler.sourcedir, suite))
        else:
            command.append(os.path.join(handler.sourcedir, self.path))

        with subprocess.Popen(command, stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT, cwd=self.instance.build_dir, env=env) as renode_test_proc:
            out, _ = renode_test_proc.communicate()

            self.instance.execution_time = time.time() - start_time

            if renode_test_proc.returncode == 0:
                self.instance.status = TwisterStatus.PASS
                # all tests in one Robot file are treated as a single test case,
                # so its status should be set accordingly to the instance status
                # please note that there should be only one testcase in testcases list
                self.instance.testcases[0].status = TwisterStatus.PASS
            else:
                logger.error(
                    f"Robot test failure: {handler.sourcedir} for {self.instance.platform.name}"
                )
                self.instance.status = TwisterStatus.FAIL
                self.instance.testcases[0].status = TwisterStatus.FAIL

            if out:
                with open(os.path.join(self.instance.build_dir, handler.log), 'w') as log:
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
        return super().get_testcase_name()

    def configure(self, instance):
        super().configure(instance)
        if self.regex is None or len(self.regex) == 0:
            self.status = TwisterStatus.FAIL
            tc = self.instance.set_case_status_by_name(
                self.get_testcase_name(),
                TwisterStatus.FAIL,
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
            self.status = TwisterStatus.FAIL
            tc = self.instance.set_case_status_by_name(
                self.get_testcase_name(),
                TwisterStatus.FAIL,
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
                self.status = TwisterStatus.PASS
        elif self.type == "multi_line" and self.ordered:
            if (self.next_pattern < len(self.patterns) and
                self.patterns[self.next_pattern].search(line)):
                logger.debug(f"HARNESS:{self.__class__.__name__}:EXPECTED("
                             f"{self.next_pattern + 1}/{self.patterns_expected}):"
                             f"'{self.patterns[self.next_pattern].pattern}'")
                self.next_pattern += 1
                if self.next_pattern >= len(self.patterns):
                    self.status = TwisterStatus.PASS
        elif self.type == "multi_line" and not self.ordered:
            for i, pattern in enumerate(self.patterns):
                r = self.regex[i]
                if pattern.search(line) and r not in self.matches:
                    self.matches[r] = line
                    logger.debug(f"HARNESS:{self.__class__.__name__}:EXPECTED("
                                 f"{len(self.matches)}/{self.patterns_expected}):"
                                 f"'{pattern.pattern}'")
            if len(self.matches) == len(self.regex):
                self.status = TwisterStatus.PASS
        else:
            logger.error("Unknown harness_config type")

        if self.fail_on_fault and self.FAULT in line:
            self.fault = True

        if self.GCOV_START in line:
            self.capture_coverage = True
        elif self.GCOV_END in line:
            self.capture_coverage = False

        self.process_test(line)
        # Reset the resulting test state to FAIL when not all of the patterns were
        # found in the output, but just ztest's 'PROJECT EXECUTION SUCCESSFUL'.
        # It might happen because of the pattern sequence diverged from the
        # test code, the test platform has console issues, or even some other
        # test image was executed.
        # TODO: Introduce explicit match policy type to reject
        # unexpected console output, allow missing patterns, deny duplicates.
        if self.status == TwisterStatus.PASS and \
           self.ordered and \
           self.next_pattern < self.patterns_expected:
            logger.error(f"HARNESS:{self.__class__.__name__}: failed with"
                         f" {self.next_pattern} of {self.patterns_expected}"
                         f" expected ordered patterns.")
            self.status = TwisterStatus.FAIL
            self.reason = "patterns did not match (ordered)"
        if self.status == TwisterStatus.PASS and \
           not self.ordered and \
           len(self.matches) < self.patterns_expected:
            logger.error(f"HARNESS:{self.__class__.__name__}: failed with"
                         f" {len(self.matches)} of {self.patterns_expected}"
                         f" expected unordered patterns.")
            self.status = TwisterStatus.FAIL
            self.reason = "patterns did not match (unordered)"

        tc = self.instance.get_case_or_create(self.get_testcase_name())
        if self.status == TwisterStatus.PASS:
            tc.status = TwisterStatus.PASS
        else:
            tc.status = TwisterStatus.FAIL


class PytestHarnessException(Exception):
    """General exception for pytest."""


class Pytest(Harness):

    def configure(self, instance: TestInstance):
        super().configure(instance)
        self.running_dir = instance.build_dir
        self.source_dir = instance.testsuite.source_dir
        self.report_file = os.path.join(self.running_dir, 'report.xml')
        self.pytest_log_file_path = os.path.join(self.running_dir, 'twister_harness.log')
        self.reserved_dut = None
        self._output = []

    def pytest_run(self, timeout):
        try:
            cmd = self.generate_command()
            self.run_command(cmd, timeout)
        except PytestHarnessException as pytest_exception:
            logger.error(str(pytest_exception))
            self.status = TwisterStatus.FAIL
            self.instance.reason = str(pytest_exception)
        finally:
            self.instance.record(self.recording)
            self._update_test_status()
            if self.reserved_dut:
                self.instance.handler.make_dut_available(self.reserved_dut)

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
            f'--platform={self.instance.platform.name}'
        ]

        command.extend([os.path.normpath(os.path.join(
            self.source_dir, os.path.expanduser(os.path.expandvars(src)))) for src in pytest_root])

        if pytest_dut_scope:
            command.append(f'--dut-scope={pytest_dut_scope}')

        # Always pass output from the pytest test and the test image up to Twister log.
        command.extend([
            '--log-cli-level=DEBUG',
            '--log-cli-format=%(levelname)s: %(message)s'
        ])

        # Use the test timeout as the base timeout for pytest
        base_timeout = handler.get_test_timeout()
        command.append(f'--base-timeout={base_timeout}')

        if handler.type_str == 'device':
            command.extend(
                self._generate_parameters_for_hardware(handler)
            )
        elif handler.type_str in SUPPORTED_SIMS_IN_PYTEST:
            command.append(f'--device-type={handler.type_str}')
        elif handler.type_str == 'build':
            command.append('--device-type=custom')
        else:
            raise PytestHarnessException(
                f'Support for handler {handler.type_str} not implemented yet'
            )

        if handler.type_str != 'device':
            for fixture in handler.options.fixture:
                command.append(f'--twister-fixture={fixture}')

        if handler.options.extra_test_args and handler.type_str == 'native':
            command.append(f'--extra-test-args={shlex.join(handler.options.extra_test_args)}')

        command.extend(pytest_args_yaml)

        if handler.options.pytest_args:
            command.extend(handler.options.pytest_args)

        return command

    def _generate_parameters_for_hardware(self, handler: Handler):
        command = ['--device-type=hardware']
        hardware = handler.get_hardware()
        if not hardware:
            raise PytestHarnessException('Hardware is not available')
        # update the instance with the device id to have it in the summary report
        self.instance.dut = hardware.id

        self.reserved_dut = hardware
        if hardware.serial_pty:
            command.append(f'--device-serial-pty={hardware.serial_pty}')
        else:
            command.extend([
                f'--device-serial={hardware.serial}',
                f'--device-serial-baud={hardware.baud}'
            ])

        if hardware.flash_timeout:
            command.append(f'--flash-timeout={hardware.flash_timeout}')

        options = handler.options
        if runner := hardware.runner or options.west_runner:
            command.append(f'--runner={runner}')

        if hardware.runner_params:
            for param in hardware.runner_params:
                command.append(f'--runner-params={param}')

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

        if hardware.flash_before:
            command.append(f'--flash-before={hardware.flash_before}')

        for fixture in hardware.fixtures:
            command.append(f'--twister-fixture={fixture}')

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
                    self.status = TwisterStatus.FAIL
                proc.wait(timeout)
            except subprocess.TimeoutExpired:
                self.status = TwisterStatus.FAIL
                proc.kill()

        if proc.returncode in (ExitCode.INTERRUPTED, ExitCode.USAGE_ERROR, ExitCode.INTERNAL_ERROR):
            self.status = TwisterStatus.ERROR
            self.instance.reason = f'Pytest error - return code {proc.returncode}'
        with open(self.pytest_log_file_path, 'w') as log_file:
            log_file.write(shlex.join(cmd) + '\n\n')
            log_file.write('\n'.join(self._output))

    @staticmethod
    def _update_command_with_env_dependencies(cmd):
        '''
        If python plugin wasn't installed by pip, then try to indicate it to
        pytest by update PYTHONPATH and append -p argument to pytest command.
        '''
        env = os.environ.copy()
        if not PYTEST_PLUGIN_INSTALLED:
            cmd.extend(['-p', 'twister_harness.plugin'])
            pytest_plugin_path = os.path.join(
                ZEPHYR_BASE,
                'scripts',
                'pylib',
                'pytest-twister-harness',
                'src'
            )
            env['PYTHONPATH'] = pytest_plugin_path + os.pathsep + env.get('PYTHONPATH', '')
            if _WINDOWS:
                cmd_append_python_path = f'set PYTHONPATH={pytest_plugin_path};%PYTHONPATH% && '
            else:
                cmd_append_python_path = (
                    f'export PYTHONPATH={pytest_plugin_path}:${{PYTHONPATH}} && '
                )
        else:
            cmd_append_python_path = ''
        cmd_to_print = cmd_append_python_path + shlex.join(cmd)
        logger.debug(f'Running pytest command: {cmd_to_print}')

        return cmd, env

    def _output_reader(self, proc):
        self._output = []
        while proc.stdout.readable() and proc.poll() is None:
            line = proc.stdout.readline().decode().strip()
            if not line:
                continue
            self._output.append(line)
            logger.debug(f'PYTEST: {line}')
            self.parse_record(line)
        proc.communicate()

    def _update_test_status(self):
        if self.status == TwisterStatus.NONE:
            self.instance.testcases = []
            try:
                self._parse_report_file(self.report_file)
            except Exception as e:
                logger.error(f'Error when parsing file {self.report_file}: {e}')
                self.status = TwisterStatus.FAIL
            finally:
                if not self.instance.testcases:
                    self.instance.init_cases()

        self.instance.status = self.status if self.status != TwisterStatus.NONE else \
                               TwisterStatus.FAIL
        if self.instance.status in [TwisterStatus.ERROR, TwisterStatus.FAIL]:
            self.instance.reason = self.instance.reason or 'Pytest failed'
            self.instance.add_missing_case_status(TwisterStatus.BLOCK, self.instance.reason)

    def _parse_report_file(self, report):
        tree = ET.parse(report)
        root = tree.getroot()

        if (elem_ts := root.find('testsuite')) is not None:
            if elem_ts.get('failures') != '0':
                self.status = TwisterStatus.FAIL
                self.instance.reason = (
                    f"{elem_ts.get('failures')}/{elem_ts.get('tests')} pytest scenario(s) failed"
                )
            elif elem_ts.get('errors') != '0':
                self.status = TwisterStatus.ERROR
                self.instance.reason = 'Error during pytest execution'
            elif elem_ts.get('skipped') == elem_ts.get('tests'):
                self.status = TwisterStatus.SKIP
            else:
                self.status = TwisterStatus.PASS
            self.instance.execution_time = float(elem_ts.get('time'))

            for elem_tc in elem_ts.findall('testcase'):
                tc = self.instance.add_testcase(f"{self.id}.{elem_tc.get('name')}")
                tc.duration = float(elem_tc.get('time'))
                elem = elem_tc.find('*')
                if elem is None:
                    tc.status = TwisterStatus.PASS
                else:
                    if elem.tag == ReportStatus.SKIP:
                        tc.status = TwisterStatus.SKIP
                    elif elem.tag == ReportStatus.FAIL:
                        tc.status = TwisterStatus.FAIL
                    else:
                        tc.status = TwisterStatus.ERROR
                    tc.reason = elem.get('message')
                    tc.output = elem.text
        else:
            self.status = TwisterStatus.SKIP
            self.instance.reason = 'No tests collected'


class Shell(Pytest):
    def generate_command(self):
        config = self.instance.testsuite.harness_config
        pytest_root = [os.path.join(ZEPHYR_BASE, 'scripts', 'pylib', 'shell-twister-harness')]
        config['pytest_root'] = pytest_root

        command = super().generate_command()
        if test_shell_file := self._get_shell_commands_file(config):
            command.append(f'--testdata={test_shell_file}')
        else:
            logger.warning('No shell commands provided')
        return command

    def _get_shell_commands_file(self, harness_config):
        if shell_commands := harness_config.get('shell_commands'):
            test_shell_file = os.path.join(self.running_dir, 'test_shell.yml')
            with open(test_shell_file, 'w') as f:
                yaml.dump(shell_commands, f)
            return test_shell_file

        test_shell_file = harness_config.get('shell_commands_file', 'test_shell.yml')
        test_shell_file = os.path.join(
            self.source_dir, os.path.expanduser(os.path.expandvars(test_shell_file))
        )
        if os.path.exists(test_shell_file):
            return test_shell_file
        return None

class Power(Pytest):
    def generate_command(self):
        config = self.instance.testsuite.harness_config
        pytest_root = [os.path.join(ZEPHYR_BASE, 'scripts', 'pylib', 'power-twister-harness')]
        config['pytest_root'] = pytest_root

        command = super().generate_command()

        if self.instance.testsuite.harness == 'power':
            measurements = config.get('power_measurements')
            command.append(f'--testdata={measurements}')
        return command

class Gtest(Harness):
    ANSI_ESCAPE = re.compile(r'\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])')
    _NAME_PATTERN = "[a-zA-Z_][a-zA-Z0-9_]*"
    _SUITE_TEST_NAME_PATTERN = f"(?P<suite_name>{_NAME_PATTERN})\\.(?P<test_name>{_NAME_PATTERN})"
    TEST_START_PATTERN = f".*\\[ RUN      \\] {_SUITE_TEST_NAME_PATTERN}"
    TEST_PASS_PATTERN = f".*\\[       OK \\] {_SUITE_TEST_NAME_PATTERN}"
    TEST_SKIP_PATTERN = f".*\\[ DISABLED \\] {_SUITE_TEST_NAME_PATTERN}"
    TEST_FAIL_PATTERN = f".*\\[  FAILED  \\] {_SUITE_TEST_NAME_PATTERN}"
    FINISHED_PATTERN = (
        ".*(?:\\[==========\\] Done running all tests\\.|"
        + "\\[----------\\] Global test environment tear-down)"
    )

    def __init__(self):
        super().__init__()
        self.tc = None
        self.has_failures = False

    def handle(self, line):
        # Strip the ANSI characters, they mess up the patterns
        non_ansi_line = self.ANSI_ESCAPE.sub('', line)

        if self.status != TwisterStatus.NONE:
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
            ), f"gTest error, {self.tc} didn't finish"

            # Check that the instance doesn't exist yet (prevents re-running)
            tc = self.instance.get_case_by_name(name)
            assert tc is None, f"gTest error, {tc} running twice"

            # Create the test instance and set the context
            tc = self.instance.get_case_or_create(name)
            self.tc = tc
            self.tc.status = TwisterStatus.STARTED
            self.testcase_output += line + "\n"
            self._match = True

        # Check if the test run finished
        finished_match = re.search(self.FINISHED_PATTERN, non_ansi_line)
        if finished_match:
            tc = self.instance.get_case_or_create(self.id)
            if self.has_failures or self.tc is not None:
                self.status = TwisterStatus.FAIL
                tc.status = TwisterStatus.FAIL
            else:
                self.status = TwisterStatus.PASS
                tc.status = TwisterStatus.PASS
            return

        # Check if the individual test finished
        state, name = self._check_result(non_ansi_line)
        if state == TwisterStatus.NONE or name is None:
            # Nothing finished, keep processing lines
            return

        # Get the matching test and make sure it's the same as the current context
        tc = self.instance.get_case_by_name(name)
        assert (
            tc is not None and tc == self.tc
        ), f"gTest error, mismatched tests. Expected {self.tc} but got {tc}"

        # Test finished, clear the context
        self.tc = None

        # Update the status of the test
        tc.status = state
        if tc.status == TwisterStatus.FAIL:
            self.has_failures = True
            tc.output = self.testcase_output
        self.testcase_output = ""
        self._match = False

    def _check_result(self, line):
        test_pass_match = re.search(self.TEST_PASS_PATTERN, line)
        if test_pass_match:
            return TwisterStatus.PASS, \
                   "{}.{}.{}".format(
                        self.id, test_pass_match.group("suite_name"),
                        test_pass_match.group("test_name")
                    )
        test_skip_match = re.search(self.TEST_SKIP_PATTERN, line)
        if test_skip_match:
            return TwisterStatus.SKIP, \
                   "{}.{}.{}".format(
                       self.id, test_skip_match.group("suite_name"),
                       test_skip_match.group("test_name")
                    )
        test_fail_match = re.search(self.TEST_FAIL_PATTERN, line)
        if test_fail_match:
            return TwisterStatus.FAIL, \
                   "{}.{}.{}".format(
                       self.id, test_fail_match.group("suite_name"),
                       test_fail_match.group("test_name")
                    )
        return None, None


class Test(Harness):
    __test__ = False  # for pytest to skip this class when collects tests

    # Ztest log patterns don't require to match the line start exactly: there are platforms
    # where there is some logging prefix at each console line whereas on other platforms
    # without prefixes the leading space is stripped.
    test_suite_start_pattern = re.compile(r"Running TESTSUITE (?P<suite_name>\S*)")
    test_suite_end_pattern = re.compile(
        r"TESTSUITE (?P<suite_name>\S*)\s+(?P<suite_status>succeeded|failed)"
    )
    test_case_start_pattern = re.compile(r"START - (test_)?([a-zA-Z0-9_-]+)")
    test_case_end_pattern = re.compile(
        r".*(PASS|FAIL|SKIP) - (test_)?(\S*) in (\d*[.,]?\d*) seconds"
    )
    test_suite_summary_pattern = re.compile(
        r"SUITE (?P<suite_status>\S*) - .* \[(?P<suite_name>\S*)\]:"
        r" .* duration = (\d*[.,]?\d*) seconds"
    )
    test_case_summary_pattern = re.compile(
        r".*- (PASS|FAIL|SKIP) - \[([^\.]*).(test_)?(\S*)\] duration = (\d*[.,]?\d*) seconds"
    )


    def get_testcase(self, tc_name, phase, ts_name=None):
        """ Search a Ztest case among detected in the test image binary
            expecting the same test names as already known from the ELF.
            Track suites and cases unexpectedly found in the log.
        """
        ts_names = self.started_suites.keys()
        if ts_name:
            if self.trace and ts_name not in self.instance.testsuite.ztest_suite_names:
                # This can happen if a ZTEST_SUITE name is macro-generated
                # in the test source files, e.g. based on DT information.
                logger.debug(f"{phase}: unexpected Ztest suite '{ts_name}' is "
                             f"not present among: {self.instance.testsuite.ztest_suite_names}")
            if ts_name not in self.detected_suite_names:
                if self.trace:
                    logger.debug(f"{phase}: detected new Ztest suite '{ts_name}'")
                self.detected_suite_names.append(ts_name)
            ts_names = [ ts_name ] if ts_name in ts_names else []

        # First, try to match the test case ID to the first running Ztest suite with this test name.
        for ts_name_ in ts_names:
            if self.started_suites[ts_name_]['count'] < (0 if phase == 'TS_SUM' else 1):
                continue
            tc_fq_id = self.instance.compose_case_name(f"{ts_name_}.{tc_name}")
            if tc := self.instance.get_case_by_name(tc_fq_id):
                if self.trace:
                    logger.debug(f"{phase}: Ztest case '{tc_name}' matched to '{tc_fq_id}")
                return tc
        logger.debug(
            f"{phase}: Ztest case '{tc_name}' is not known"
            f" in {self.started_suites} running suite(s)."
        )
        tc_id = self.instance.compose_case_name(tc_name)
        return self.instance.get_case_or_create(tc_id)

    def start_suite(self, suite_name, phase='TS_START'):
        if suite_name not in self.detected_suite_names:
            self.detected_suite_names.append(suite_name)
        if self.trace and suite_name not in self.instance.testsuite.ztest_suite_names:
            # This can happen if a ZTEST_SUITE name is macro-generated
            # in the test source files, e.g. based on DT information.
            logger.debug(f"{phase}: unexpected Ztest suite '{suite_name}' is "
                         f"not present among: {self.instance.testsuite.ztest_suite_names}")
        if suite_name in self.started_suites:
            if self.started_suites[suite_name]['count'] > 0 and not self.expect_reboot:
                # Either the suite restarts itself or unexpected state transition.
                logger.warning(f"{phase}: already STARTED '{suite_name}':"
                               f"{self.started_suites[suite_name]}")
            elif self.trace:
                logger.debug(f"{phase}: START suite '{suite_name}'")
            self.started_suites[suite_name]['count'] += 1
            self.started_suites[suite_name]['repeat'] += 1
        else:
            self.started_suites[suite_name] = { 'count': 1, 'repeat': 0 }

    def end_suite(self, suite_name, phase='TS_END', suite_status=None):
        if suite_name in self.started_suites:
            if phase == 'TS_SUM' and self.started_suites[suite_name]['count'] == 0:
                return
            if self.started_suites[suite_name]['count'] < 1:
                logger.error(
                    f"{phase}: already ENDED suite '{suite_name}':{self.started_suites[suite_name]}"
                )
            elif self.trace:
                logger.debug(f"{phase}: END suite '{suite_name}':{self.started_suites[suite_name]}")
            self.started_suites[suite_name]['count'] -= 1
        elif suite_status == 'SKIP':
            self.start_suite(suite_name, phase)  # register skipped suites at their summary end
            self.started_suites[suite_name]['count'] -= 1
        else:
            logger.warning(f"{phase}: END suite '{suite_name}' without START detected")

    def start_case(self, tc_name, phase='TC_START'):
        if tc_name in self.started_cases:
            if self.started_cases[tc_name]['count'] > 0 and not self.expect_reboot:
                logger.warning(f"{phase}: already STARTED case "
                               f"'{tc_name}':{self.started_cases[tc_name]}")
            self.started_cases[tc_name]['count'] += 1
        else:
            self.started_cases[tc_name] = { 'count': 1 }

    def end_case(self, tc_name, phase='TC_END'):
        if tc_name in self.started_cases:
            if phase == 'TS_SUM' and self.started_cases[tc_name]['count'] == 0:
                return
            if self.started_cases[tc_name]['count'] < 1:
                logger.error(
                    f"{phase}: already ENDED case '{tc_name}':{self.started_cases[tc_name]}"
                )
            elif self.trace:
                logger.debug(f"{phase}: END case '{tc_name}':{self.started_cases[tc_name]}")
            self.started_cases[tc_name]['count'] -= 1
        elif phase != 'TS_SUM':
            logger.warning(f"{phase}: END case '{tc_name}' without START detected")

    def handle(self, line):
        testcase_match = None
        if self._match:
            self.testcase_output += line + "\n"
        if test_suite_start_match := re.search(self.test_suite_start_pattern, line):
            self.start_suite(test_suite_start_match.group("suite_name"))
        elif test_suite_end_match := re.search(self.test_suite_end_pattern, line):
            suite_name=test_suite_end_match.group("suite_name")
            self.end_suite(suite_name)
            self.ztest = True
        elif testcase_match := re.search(self.test_case_start_pattern, line):
            tc_name = testcase_match.group(2)
            tc = self.get_testcase(tc_name, 'TC_START')
            self.start_case(tc.name)
            # Mark the test as started, if something happens here, it is mostly
            # due to this tests, for example timeout. This should in this case
            # be marked as failed and not blocked (not run).
            tc.status = TwisterStatus.STARTED
            if not self._match:
                self.testcase_output += line + "\n"
                self._match = True
        # some testcases are skipped based on predicates and do not show up
        # during test execution, however they are listed in the summary. Parse
        # the summary for status and use that status instead.
        elif result_match := self.test_case_end_pattern.match(line):
            matched_status = result_match.group(1)
            tc_name = result_match.group(3)
            tc = self.get_testcase(tc_name, 'TC_END')
            self.end_case(tc.name)
            tc.status = TwisterStatus[matched_status]
            if tc.status == TwisterStatus.SKIP:
                tc.reason = "ztest skip"
            tc.duration = float(result_match.group(4))
            if tc.status == TwisterStatus.FAIL:
                tc.output = self.testcase_output
            self.testcase_output = ""
            self._match = False
            self.ztest = True
        elif test_suite_summary_match := self.test_suite_summary_pattern.match(line):
            suite_name=test_suite_summary_match.group("suite_name")
            suite_status=test_suite_summary_match.group("suite_status")
            self._match = False
            self.ztest = True
            self.end_suite(suite_name, 'TS_SUM', suite_status=suite_status)
        elif test_case_summary_match := self.test_case_summary_pattern.match(line):
            matched_status = test_case_summary_match.group(1)
            suite_name = test_case_summary_match.group(2)
            tc_name = test_case_summary_match.group(4)
            tc = self.get_testcase(tc_name, 'TS_SUM', suite_name)
            self.end_case(tc.name, 'TS_SUM')
            if tc.status not in [TwisterStatus.NONE, TwisterStatus[matched_status]]:
                # TestCase miss its END log entry, so its status is from the Suite summary.
                logger.warning(
                    f"TS_SUM: {tc.name} force status: {tc.status}->{TwisterStatus[matched_status]}"
                )
            tc.status = TwisterStatus[matched_status]
            if tc.status == TwisterStatus.SKIP:
                tc.reason = "ztest skip"
            tc.duration = float(test_case_summary_match.group(5))
            if tc.status == TwisterStatus.FAIL:
                tc.output = self.testcase_output
            self.testcase_output = ""
            self._match = False
            self.ztest = True

        self.process_test(line)

        if not self.ztest and self.status != TwisterStatus.NONE:
            logger.debug(f"{self.id} is not a Ztest, status:{self.status}")
            tc = self.instance.get_case_or_create(self.id)
            if self.status == TwisterStatus.PASS:
                tc.status = TwisterStatus.PASS
            else:
                tc.status = TwisterStatus.FAIL
                tc.reason = "Test failure"


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
            new_exe_name = f'bs_{new_exe_name}'

        new_exe_name = new_exe_name.replace(os.path.sep, '_').replace('.', '_').replace('@', '_')

        new_exe_path: str = os.path.join(bsim_out_path, 'bin', new_exe_name)
        logger.debug(f'Copying executable from {original_exe_path} to {new_exe_path}')
        shutil.copy(original_exe_path, new_exe_path)

class Ctest(Harness):
    def configure(self, instance: TestInstance):
        super().configure(instance)
        self.running_dir = instance.build_dir
        self.report_file = os.path.join(self.running_dir, 'report.xml')
        self.ctest_log_file_path = os.path.join(self.running_dir, 'twister_harness.log')
        self._output = []

    def ctest_run(self, timeout):
        assert self.instance is not None
        try:
            cmd = self.generate_command()
            self.run_command(cmd, timeout)
        except Exception as err:
            logger.error(str(err))
            self.status = TwisterStatus.FAIL
            self.instance.reason = str(err)
        finally:
            self.instance.record(self.recording)
            self._update_test_status()

    def generate_command(self):
        config = self.instance.testsuite.harness_config
        handler: Handler = self.instance.handler
        ctest_args_yaml = config.get('ctest_args', []) if config else []
        command = [
            'ctest',
            '--build-nocmake',
            '--test-dir',
            self.running_dir,
            '--output-junit',
            self.report_file,
            '--output-log',
            self.ctest_log_file_path,
            '--output-on-failure',
        ]
        base_timeout = handler.get_test_timeout()
        command.extend(['--timeout', str(base_timeout)])
        command.extend(ctest_args_yaml)

        if handler.options.ctest_args:
            command.extend(handler.options.ctest_args)

        return command

    def run_command(self, cmd, timeout):
        with subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        ) as proc:
            try:
                reader_t = threading.Thread(target=self._output_reader, args=(proc,), daemon=True)
                reader_t.start()
                reader_t.join(timeout)
                if reader_t.is_alive():
                    terminate_process(proc)
                    logger.warning('Timeout has occurred. Can be extended in testspec file. '
                                   f'Currently set to {timeout} seconds.')
                    self.instance.reason = 'Ctest timeout'
                    self.status = TwisterStatus.FAIL
                proc.wait(timeout)
            except subprocess.TimeoutExpired:
                self.status = TwisterStatus.FAIL
                proc.kill()

        if proc.returncode in (ExitCode.INTERRUPTED, ExitCode.USAGE_ERROR, ExitCode.INTERNAL_ERROR):
            self.status = TwisterStatus.ERROR
            self.instance.reason = f'Ctest error - return code {proc.returncode}'
            with open(self.ctest_log_file_path, 'w') as log_file:
                log_file.write(shlex.join(cmd) + '\n\n')
                log_file.write('\n'.join(self._output))

    def _output_reader(self, proc):
        self._output = []
        while proc.stdout.readable() and proc.poll() is None:
            line = proc.stdout.readline().decode().strip()
            if not line:
                continue
            self._output.append(line)
            logger.debug(f'CTEST: {line}')
            self.parse_record(line)
        proc.communicate()

    def _update_test_status(self):
        if self.status == TwisterStatus.NONE:
            self.instance.testcases = []
            try:
                self._parse_report_file(self.report_file)
            except Exception as e:
                logger.error(f'Error when parsing file {self.report_file}: {e}')
                self.status = TwisterStatus.FAIL
            finally:
                if not self.instance.testcases:
                    self.instance.init_cases()

        self.instance.status = self.status if self.status != TwisterStatus.NONE else \
                               TwisterStatus.FAIL
        if self.instance.status in [TwisterStatus.ERROR, TwisterStatus.FAIL]:
            self.instance.reason = self.instance.reason or 'Ctest failed'
            self.instance.add_missing_case_status(TwisterStatus.BLOCK, self.instance.reason)

    def _parse_report_file(self, report):
        suite = junit.JUnitXml.fromfile(report)
        if suite is None:
            self.status = TwisterStatus.SKIP
            self.instance.reason = 'No tests collected'
            return

        assert isinstance(suite, junit.TestSuite)

        if suite.failures and suite.failures > 0:
            self.status = TwisterStatus.FAIL
            self.instance.reason = f"{suite.failures}/{suite.tests} ctest scenario(s) failed"
        elif suite.errors and suite.errors > 0:
            self.status = TwisterStatus.ERROR
            self.instance.reason = 'Error during ctest execution'
        elif suite.skipped and suite.skipped > 0:
            self.status = TwisterStatus.SKIP
        else:
            self.status = TwisterStatus.PASS
        self.instance.execution_time = suite.time

        for case in suite:
            tc = self.instance.add_testcase(f"{self.id}.{case.name}")
            tc.duration = case.time
            if any(isinstance(r, junit.Failure) for r in case.result):
                tc.status = TwisterStatus.FAIL
                tc.output = case.system_out
            elif any(isinstance(r, junit.Error) for r in case.result):
                tc.status = TwisterStatus.ERROR
                tc.output = case.system_out
            elif any(isinstance(r, junit.Skipped) for r in case.result):
                tc.status = TwisterStatus.SKIP
            else:
                tc.status = TwisterStatus.PASS

class HarnessImporter:

    @staticmethod
    def get_harness(harness_name):
        thismodule = sys.modules[__name__]
        try:
            if harness_name:
                harness_class = getattr(thismodule, harness_name)
            else:
                harness_class = thismodule.Test
            return harness_class()
        except AttributeError as e:
            logger.debug(f"harness {harness_name} not implemented: {e}")
            return None
