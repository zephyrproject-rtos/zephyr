# Copyright (c) 2026 Analog Devices
# SPDX-License-Identifier: Apache-2.0

from .clock_driver import ClockDriver


class FixedFactorClockDriver(ClockDriver):
    """
    Clock driver for fixed-factor-clock nodes.
    These apply a fixed multiplier and divider to the parent clock rate.
    Output: parent_rate * multiplier / divider.
    No specifiers from consumers (#clock-cells = <0>).
    """

    @classmethod
    def get_dt_compat(cls):
        return "fixed-factor-clock"

    def __init__(self, dt_node):
        super().__init__(dt_node)
        self.divider = dt_node.props.get('clock-div', None)
        self.divider = self.divider.val if self.divider is not None else 1
        self.multiplier = dt_node.props.get('clock-mult', None)
        self.multiplier = self.multiplier.val if self.multiplier is not None else 1
        self.parent = dt_node.props['clocks'].val[0].controller

    def default_specifiers(self):
        return {}

    def get_parents(self):
        return [self.parent]

    def configure_recalc(self, specifiers: dict[str, int], parent_rate: int):
        if self.divider == 0 or self.multiplier == 0:
            return -1
        return (parent_rate * self.multiplier) // self.divider
