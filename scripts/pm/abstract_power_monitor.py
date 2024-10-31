# Copyright (c) 2024 Intel Corporation.
# SPDX-License-Identifier: Apache-2.0
from abc import ABC, abstractmethod
import string
from typing import List

class Power_Monitor(ABC):

    @abstractmethod
    def init_power_monitor(self, device_id:string):
        """
        Abstract method to initialize the power monitor.

        Agr:
            string: Address of the power monitor

        Return:
            bool: True of False.
        """

    @abstractmethod
    def measure_current(self, duration:int):
        """
        Abstract method to measure current with specified measurement time.

        Args:
            duration (int): The duration of the measurement in seconds.
        """

    @abstractmethod
    def get_data(self, duration: int) -> List[float]:
        """
        Measure current with specified measurement time.

        Args:
            duration (int): The duration of the measurement in seconds.

        Returns:
            List[float]: An array of measured current values in amperes.
        """
