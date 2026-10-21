# Copyright (c) 2026 Analog Devices
# SPDX-License-Identifier: Apache-2.0

from .clock_driver import ClockDriver


class ClockSourceDriver(ClockDriver):
    """
    Clock driver for clock-source nodes. These are root clock sources
    with a static frequency set via the 'clock-frequency' DT property.
    clock-source has #clock-cells = <1>, which enables gating the clock
    """

    @classmethod
    def get_dt_compat(cls):
        return "clock-source"

    def __init__(self, dt_node):
        super().__init__(dt_node)
        self.frequency = dt_node.props['clock-frequency'].val

    def default_specifiers(self):
        return {'gate': 0}

    def get_parents(self):
        return None

    def root_recalc(self, specifiers: dict[str, int]):
        return self.frequency if specifiers['gate'] == 1 else 0
