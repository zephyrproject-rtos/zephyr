# Copyright (c) 2026 Analog Devices
# SPDX-License-Identifier: Apache-2.0

from common import ClockDriver


class NxpSysconFlexFrgDriver(ClockDriver):
    """
    Clock driver for nxp,syscon-flexfrg (Flexcomm Fractional Rate Divider).
    The specifier 'numerator' sets the MULT field (0-255).
    The DIV field is fixed at 255 (0xFF).
    Output frequency: parent_rate * 256 / (numerator + 256)

    This means the output ranges from parent_rate (when numerator=0)
    down to parent_rate/2 (when numerator=255).
    """

    DIV_MASK = 0xFF

    @classmethod
    def get_dt_compat(cls):
        return "nxp,syscon-flexfrg"

    def __init__(self, dt_node):
        super().__init__(dt_node)
        self.parent = dt_node.props['input'].val

    def get_parents(self):
        return [self.parent]

    def default_specifiers(self):
        # Assumption: MULT register defaults to 0 at reset,
        # meaning no fractional division (output = input).
        return {'numerator': 0}

    def configure_recalc(self, specifiers: dict[str, int], parent_rate: int):
        numerator = specifiers['numerator']
        if numerator < 0 or numerator > self.DIV_MASK:
            return -1
        return (parent_rate * (self.DIV_MASK + 1)) // (numerator + self.DIV_MASK + 1)
