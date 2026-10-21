# Copyright (c) 2026 Analog Devices
# SPDX-License-Identifier: Apache-2.0

from common import ClockDriver


class NxpSysconRtcClkDriver(ClockDriver):
    """
    Clock driver for nxp,syscon-rtcclk nodes.
    These are RTC clock dividers with an add-factor that offsets the
    register value. The specifier 'divider' sets the total division
    factor applied to the parent clock: output = parent_rate / divider.

    The hardware register stores (divider - add_factor), but for the
    configure_recalc the specifier represents the actual division factor.
    """

    @classmethod
    def get_dt_compat(cls):
        return "nxp,syscon-rtcclk"

    def __init__(self, dt_node):
        super().__init__(dt_node)
        self.add_factor = dt_node.props['add-factor'].val
        self.mask_width = dt_node.props['reg'].val[1]
        self.max_reg_val = (1 << self.mask_width) - 1
        self.parent = dt_node.props['input'].val

    def get_parents(self):
        return [self.parent]

    def default_specifiers(self):
        # Assumption: register value defaults to 0 at reset.
        # Actual divider = reg_val + add_factor = 0 + add_factor.
        return {'divider': self.add_factor}

    def configure_recalc(self, specifiers: dict[str, int], parent_rate: int):
        divider = specifiers['divider']
        if divider <= 0:
            return -1
        reg_val = divider - self.add_factor
        if reg_val < 0 or reg_val > self.max_reg_val:
            return -1
        return parent_rate // divider
