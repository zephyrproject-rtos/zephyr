from abc import ABC, abstractmethod
from twisterlib.testplan import TestSuite


class FilterFramework(ABC):
    def setup(self, *args, **kwargs):
        pass

    @abstractmethod
    def filter(self, suite: TestSuite) -> bool:
        """
        Examines a given Test Suite and decides if it should be run or not.
        Returns True if the suite has to be excluded, False if it's supposed to be executed.
        """
        pass

    def teardown(self):
        pass
