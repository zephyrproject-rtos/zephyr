# SPDX-License-Identifier: Apache-2.0
import re
from collections import OrderedDict

result_re = re.compile("(PASS|FAIL|SKIP) - (test_)?(.*)")

class Harness:
    GCOV_START = "GCOV_COVERAGE_DUMP_START"
    GCOV_END = "GCOV_COVERAGE_DUMP_END"
    RUN_PASSED = "PROJECT EXECUTION SUCCESSFUL"
    RUN_FAILED = "PROJECT EXECUTION FAILED"

    FAULTS = [
            "Unknown Fatal Error",
            "MPU FAULT",
            "Kernel Panic",
            "Kernel OOPS",
            "BUS FAULT",
            "CPU Page Fault"
            ]

    def __init__(self):
        self.state = None
        self.type = None
        self.regex = []
        self.matches = OrderedDict()
        self.ordered = True
        self.repeat = 1
        self.tests = {}
        self.id = None
        self.fail_on_fault = True
        self.fault = False
        self.capture_coverage = False
        self.record = None
        self.recording = []
        self.fieldnames = []

    def configure(self, instance):
        config = instance.test.harness_config
        self.id = instance.test.id
        if "ignore_faults" in instance.test.tags:
            self.fail_on_fault = False

        if config:
            self.type = config.get('type', None)
            self.regex = config.get('regex', [] )
            self.repeat = config.get('repeat', 1)
            self.ordered = config.get('ordered', True)
            self.record = config.get('record', {})

    def process_test(self, line):

        if self.RUN_PASSED in line:
            if self.fault:
                self.state = "failed"
            else:
                self.state = "passed"

        if self.RUN_FAILED in line:
            self.state = "failed"

        if self.fail_on_fault:
            for fault in self.FAULTS:
                if fault in line:
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
        elif self.type == "multi_line":
            for i, pattern in enumerate(self.patterns):
                r = self.regex[i]
                if pattern.search(line) and not r in self.matches:
                    self.matches[r] = line

            if len(self.matches) == len(self.regex):
                # check ordering
                if self.ordered:
                    ordered = True
                    pos = 0
                    for k in self.matches:
                        if k != self.regex[pos]:
                            ordered = False
                        pos += 1

                    if ordered:
                        self.state = "passed"
                    else:
                        self.state = "failed"
                else:
                    self.state = "passed"

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

        if self.state == "passed":
            self.tests[self.id] = "PASS"
        else:
            self.tests[self.id] = "FAIL"

        self.process_test(line)

class Test(Harness):
    RUN_PASSED = "PROJECT EXECUTION SUCCESSFUL"
    RUN_FAILED = "PROJECT EXECUTION FAILED"

    def handle(self, line):
        match = result_re.match(line)
        if match:
            name = "{}.{}".format(self.id, match.group(3))
            self.tests[name] = match.group(1)

        self.process_test(line)
