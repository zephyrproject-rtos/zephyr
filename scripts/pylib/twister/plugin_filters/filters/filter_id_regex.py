#!/usr/bin/env python3
#
# Copyright (c) 2025 Baumer Group. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

import re

from plugin_filters.filter_interface import FilterInterface
from twisterlib.testplan import TestSuite


class Filter(FilterInterface):
    regex_pattern = None

    def setup(self, regex_pattern, *args, **kwargs):
        self.regex_pattern = regex_pattern

    def exclude(self, suite: TestSuite):
        return not bool(re.search(self.regex_pattern, suite.id))
