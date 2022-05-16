# SPDX-License-Identifier: Apache-2.0
from asyncio.log import logger
import re
import os
import subprocess
from collections import OrderedDict
import xml.etree.ElementTree as ET
import logging

logger = logging.getLogger('twister')
logger.setLevel(logging.DEBUG)

# pylint: disable=anomalous-backslash-in-string
result_re = re.compile(".*(PASS|FAIL|SKIP) - (test_)?(.*) in (\d*[.,]?\d*) seconds")
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
        self.testcases = []
        self.id = None
        self.fail_on_fault = True
        self.fault = False
        self.capture_coverage = False
        self.next_pattern = 0
        self.record = None
        self.recording = []
        self.fieldnames = []
        self.ztest = False
        self.is_pytest = False
        self.detected_suite_names = []
        self.run_id = None
        self.matched_run_id = False
        self.run_id_exists = False
        self.instance = None
        self.testcase_output = ""
        self._match = False

    def configure(self, instance):
        self.instance = instance
        config = instance.testsuite.harness_config
        self.id = instance.testsuite.id
        self.run_id = instance.run_id
        if "ignore_faults" in instance.testsuite.tags:
            self.fail_on_fault = False

        if config:
            self.type = config.get('type', None)
            self.regex = config.get('regex', [])
            self.repeat = config.get('repeat', 1)
            self.ordered = config.get('ordered', True)
            self.record = config.get('record', {})

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

class Console(Harness):

    def configure(self, instance):
        super(Console, self).configure(instance)
        if self.type == "one_line":
            self.pattern = re.compile(self.regex[0])
        elif self.type == "multi_line":
            self.patterns = []
            for r in self.regex:
                self.patterns.append(re.compile(r))

    def handle(self, line):
        if self.type == "one_line":
            if self.pattern.search(line):
                self.state = "passed"
        elif self.type == "multi_line" and self.ordered:
            if (self.next_pattern < len(self.patterns) and
                self.patterns[self.next_pattern].search(line)):
                self.next_pattern += 1
                if self.next_pattern >= len(self.patterns):
                    self.state = "passed"
        elif self.type == "multi_line" and not self.ordered:
            for i, pattern in enumerate(self.patterns):
                r = self.regex[i]
                if pattern.search(line) and not r in self.matches:
                    self.matches[r] = line
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


        if self.record:
            pattern = re.compile(self.record.get("regex", ""))
            match = pattern.search(line)
            if match:
                csv = []
                if not self.fieldnames:
                    for k,v in match.groupdict().items():
                        self.fieldnames.append(k)

                for k,v in match.groupdict().items():
                    csv.append(v.strip())
                self.recording.append(csv)

        self.process_test(line)

        tc = self.instance.get_case_or_create(self.id)
        if self.state == "passed":
            tc.status = "passed"
        else:
            tc.status = "failed"

class Pytest(Harness):
    def configure(self, instance):
        super(Pytest, self).configure(instance)
        self.running_dir = instance.build_dir
        self.source_dir = instance.testsuite.source_dir
        self.pytest_root = 'pytest'
        self.pytest_args = []
        self.is_pytest = True
        config = instance.testsuite.harness_config

        if config:
            self.pytest_root = config.get('pytest_root', 'pytest')
            self.pytest_args = config.get('pytest_args', [])

    def handle(self, line):
        ''' Test cases that make use of pytest more care about results given
            by pytest tool which is called in pytest_run(), so works of this
            handle is trying to give a PASS or FAIL to avoid timeout, nothing
            is writen into handler.log
        '''
        self.state = "passed"
        tc = self.instance.get_case_or_create(self.id)
        tc.status = "passed"

    def pytest_run(self, log_file):
        ''' To keep artifacts of pytest in self.running_dir, pass this directory
            by "--cmdopt". On pytest end, add a command line option and provide
            the cmdopt through a fixture function
            If pytest harness report failure, twister will direct user to see
            handler.log, this method writes test result in handler.log
        '''
        cmd = [
			'pytest',
			'-s',
			os.path.join(self.source_dir, self.pytest_root),
			'--cmdopt',
			self.running_dir,
			'--junit-xml',
			os.path.join(self.running_dir, 'report.xml'),
			'-q'
        ]

        for arg in self.pytest_args:
            cmd.append(arg)

        log = open(log_file, "a")
        outs = []
        errs = []

        with subprocess.Popen(cmd,
                              stdout = subprocess.PIPE,
                              stderr = subprocess.PIPE) as proc:
            try:
                outs, errs = proc.communicate()
                tree = ET.parse(os.path.join(self.running_dir, "report.xml"))
                root = tree.getroot()
                for child in root:
                    if child.tag == 'testsuite':
                        if child.attrib['failures'] != '0':
                            self.state = "failed"
                        elif child.attrib['skipped'] != '0':
                            self.state = "skipped"
                        elif child.attrib['errors'] != '0':
                            self.state = "errors"
                        else:
                            self.state = "passed"
            except subprocess.TimeoutExpired:
                proc.kill()
                self.state = "failed"
            except ET.ParseError:
                self.state = "failed"
            except IOError:
                log.write("Can't access report.xml\n")
                self.state = "failed"

        tc = self.instance.get_case_or_create(self.id)
        if self.state == "passed":
            tc.status = "passed"
            log.write("Pytest cases passed\n")
        elif self.state == "skipped":
            tc.status = "skipped"
            log.write("Pytest cases skipped\n")
            log.write("Please refer report.xml for detail")
        else:
            tc.status = "failed"
            log.write("Pytest cases failed\n")

        log.write("\nOutput from pytest:\n")
        log.write(outs.decode('UTF-8'))
        log.write(errs.decode('UTF-8'))
        log.close()


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
        if testcase_match or self._match:
            self.testcase_output += line + "\n"
            self._match = True

        match = result_re.match(line)

        if match and match.group(2):
            name = "{}.{}".format(self.id, match.group(3))
            tc = self.instance.get_case_or_create(name)

            matched_status = match.group(1)
            tc.status = self.ztest_to_status[matched_status]
            if tc.status == "skipped":
                tc.reason = "ztest skip"
            tc.duration = float(match.group(4))
            if tc.status == "failed":
                tc.output = self.testcase_output
            self.testcase_output = ""
            self._match = False
            self.ztest = True

        self.process_test(line)

        if not self.ztest and self.state:
            tc = self.instance.get_case_or_create(self.id)
            if self.state == "passed":
                tc.status = "passed"
            else:
                tc.status = "failed"

class Ztest(Test):
    pass
