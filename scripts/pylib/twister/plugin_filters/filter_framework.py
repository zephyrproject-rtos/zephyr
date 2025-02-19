from abc import ABC, abstractmethod
from twisterlib.testplan import TestSuite


class FilterFramework(ABC):
    def setup(self):
        pass

    @abstractmethod
    def filter(self, suite: TestSuite):
        pass

    def teardown(self):
        pass
