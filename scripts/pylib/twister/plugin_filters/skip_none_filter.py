from twisterlib.testplan import TestSuite
from plugin_filters.filter_framework import FilterFramework


class Filter(FilterFramework):
    def filter(self, suite: TestSuite):
        return False
