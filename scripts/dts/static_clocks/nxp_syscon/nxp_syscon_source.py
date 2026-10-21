# Copyright (c) 2026 Analog Devices
# SPDX-License-Identifier: Apache-2.0

from common import ClockDriver


class NxpSysconClockSourceDriver(ClockDriver):
    """
    Clock driver for nxp,syscon-clock-source nodes.
    These are root clock sources with a fixed frequency and a gate specifier.
    Specifier 'gate': 1 to enable the source (returns configured frequency),
    0 to disable (returns 0).
    """

    @classmethod
    def get_dt_compat(cls):
        return "nxp,syscon-clock-source"

    def __init__(self, dt_node):
        super().__init__(dt_node)
        self.frequency = dt_node.props['frequency'].val

    def get_parents(self):
        return None

    def default_specifiers(self):
        # Assumption: clock sources default to gated (disabled) at reset.
        # The actual reset state depends on the enable bit in the register,
        # but conservatively assume disabled.
        return {'gate': 0}

    def root_recalc(self, specifiers: dict[str, int]):
        if specifiers['gate']:
            return self.frequency
        return 0
