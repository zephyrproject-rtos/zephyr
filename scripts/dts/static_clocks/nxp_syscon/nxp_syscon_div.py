# Copyright (c) 2026 Analog Devices
# SPDX-License-Identifier: Apache-2.0

from common import ClockDriver


class NxpSysconDivClockDriver(ClockDriver):
    """
    Clock driver for NXP SYSCON divider clocks.
    """

    def __init__(self, dt_node):
        super().__init__(dt_node)
        mask_width = dt_node.props['reg'].val[1]
        self.max_value = 1 << mask_width
        self.parent = dt_node.props['input'].val

    @classmethod
    def get_dt_compat(cls):
        """
        Get the device tree compatible string for this clock driver.
        """
        return "nxp,syscon-clock-div"

    def default_specifiers(self):
        """
        Get the default specifiers for this clock driver.
        """
        return {'divider': 1}

    def configure_recalc(self, specifiers: dict[str, int], parent_rate: int):
        """
        recalculate the clock rate based on the given specifiers and parent rate.
        @param specifiers: the specifiers used to configure the clock.
        @param parent_rate: the rate of the parent clock.
        @return: the recalculated clock rate, or a negative value if the specifier is invalid.
        """
        if (
            'divider' not in specifiers
            or specifiers['divider'] <= 0
            or specifiers['divider'] > self.max_value
        ):
            return -1
        return parent_rate // specifiers['divider']

    def get_parents(self):
        return [self.parent]
