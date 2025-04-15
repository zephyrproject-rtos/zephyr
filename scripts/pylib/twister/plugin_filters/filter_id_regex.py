import re

from twisterlib.testplan import TestSuite
from plugin_filters.filter_framework import FilterFramework


class Filter(FilterFramework):
    def setup(self, regex_pattern):
        self.regex_pattern = regex_pattern

    def filter(self, suite: TestSuite):
        return not bool(re.search(self.regex_pattern, suite.id))
