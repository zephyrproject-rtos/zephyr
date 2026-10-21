# Copyright (c) 2026 Analog Devices
# SPDX-License-Identifier: Apache-2.0

from .clock_driver import ClockDriver


class FixedClockDriver(ClockDriver):
    """
    Clock driver for fixed-clock nodes. These are root clock sources
    with a static frequency set via the 'clock-frequency' DT property.
    fixed-clock has #clock-cells = <0>, so consumers pass no specifiers.
    """

    @classmethod
    def get_dt_compat(cls):
        return "fixed-clock"

    def __init__(self, dt_node):
        super().__init__(dt_node)
        self.frequency = dt_node.props['clock-frequency'].val

    def default_specifiers(self):
        return {}

    def get_parents(self):
        return None

    def root_recalc(self, specifiers: dict[str, int]):
        return self.frequency
