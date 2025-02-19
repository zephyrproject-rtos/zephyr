from twisterlib.testplan import TestSuite
from plugin_filters.filter_framework import FilterFramework


class Filter(FilterFramework):
    def setup(self, id_filter='sample.kernel.philosopher.semaphores'):
        self.id_filter = id_filter

    def filter(self, suite: TestSuite):
        return suite.id != self.id_filter
