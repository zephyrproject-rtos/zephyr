import re

from twisterlib.testplan import TestSuite
from plugin_filters.filter_interface import FilterInterface


class Filter(FilterInterface):
    regex_pattern = None

    def setup(self, regex_pattern, *args, **kwargs):
        self.regex_pattern = regex_pattern

    def exclude(self, suite: TestSuite):
        return not bool(re.search(self.regex_pattern, suite.id))
