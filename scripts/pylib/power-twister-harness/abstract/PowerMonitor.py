# Copyright: (c)  2025, Intel Corporation
# Author: Arkadiusz Cholewinski <arkadiuszx.cholewinski@intel.com>

import string
from abc import ABC, abstractmethod


class PowerMonitor(ABC):
    @abstractmethod
    def init(self, device_id: string):
        """
        Abstract method to initialize the power monitor.

        Agr:
            string: Address of the power monitor

        Return:
            bool: True of False.
        """

    @abstractmethod
    def measure(self, duration: int):
        """
        Abstract method to measure current with specified measurement time.

        Args:
            duration (int): The duration of the measurement in seconds.
        """

    @abstractmethod
    def get_data(self, duration: int) -> list[float]:
        """
        Measure current with specified measurement time.

        Args:
            duration (int): The duration of the measurement in seconds.

        Returns:
            List[float]: An array of measured current values in amperes.
        """
