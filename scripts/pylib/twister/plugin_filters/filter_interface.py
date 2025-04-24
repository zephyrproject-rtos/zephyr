from abc import ABC, abstractmethod


class FilterInterface(ABC):
    def setup(self, *args, **kwargs):
        pass

    @abstractmethod
    def exclude(self, suite) -> bool:
        """
        Examines a given Test Suite and decides if it should be run or not.
        Returns True if the suite has to be excluded, False if it's supposed to be executed.
        """
        pass

    def teardown(self):
        pass
