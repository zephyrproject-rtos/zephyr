# Copyright (c) 2026 Analog Devices
# SPDX-License-Identifier: Apache-2.0

from .clock_driver import ClockDriver


class ClockGateDriver(ClockDriver):
    """
    Clock driver for generic clock-gate nodes.
    Specifier 'gate': 1 to ungate (pass parent rate through), 0 to gate.
    """

    @classmethod
    def get_dt_compat(cls):
        return "clock-gate"

    def __init__(self, dt_node):
        super().__init__(dt_node)
        self.parent = dt_node.props['input'].val

    def get_parents(self):
        return [self.parent]

    def default_specifiers(self):
        # Assumption: gates default to gated (0) at reset.
        return {'gate': 0}

    def configure_recalc(self, specifiers: dict[str, int], parent_rate: int):
        if specifiers['gate']:
            return parent_rate
        return 0
