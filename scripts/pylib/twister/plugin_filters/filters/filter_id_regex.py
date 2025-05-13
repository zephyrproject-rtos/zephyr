#!/usr/bin/env python3
#
# Copyright (c) 2025 Baumer Group. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

import re

from plugin_filters.filter_interface import FilterInterface
from twisterlib.testplan import TestSuite


class Filter(FilterInterface):
    """
    The Filter ID Regex excludes Suites based on their ID.
    """

    regex_pattern: str = "*"

    # pylint: disable=arguments-differ
    def setup(self, regex_pattern: str, *args, **kwargs):
        super().setup(*args, **kwargs)

        self.regex_pattern = regex_pattern

    def exclude(self, suite: TestSuite):
        # Example: the regex_pattern "sample\\.bluetooth\\..*" would skip all but those test cases
        # from testcase.yaml or sample.yaml resp. the testsuites already collected in the testplan
        # that start with sample.bluetooth.
        return not bool(re.search(self.regex_pattern, suite.id))

    def include(self, suite: TestSuite):
        # Example: the regex_pattern "sample\\.bluetooth\\..*" would include only those test cases
        # from testcase.yaml or sample.yaml resp. the testsuites already collected in the testplan
        # that start with sample.bluetooth.
        return bool(re.search(self.regex_pattern, suite.id))
