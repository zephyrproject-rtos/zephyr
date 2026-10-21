# Copyright (c) 2026 Analog Devices
# SPDX-License-Identifier: Apache-2.0

from .clock_driver import ClockDriver


class ClockMuxDriver(ClockDriver):
    """
    Clock driver for clock-mux nodes.
    Specifier 'mux': zero-based index into the input-sources phandle
    array selecting which parent clock to use.
    """

    @classmethod
    def get_dt_compat(cls):
        return "clock-mux"

    def __init__(self, dt_node):
        super().__init__(dt_node)
        self.parent_count = len(dt_node.props['input-sources'].val)
        self.parents = dt_node.props['input-sources'].val

    def default_specifiers(self):
        return {'mux': 0}

    def mux_recalc(self, specifiers: dict[str, int]):
        idx = specifiers['mux']
        if idx < 0 or idx >= self.parent_count:
            return -1
        return idx

    def get_parents(self):
        return self.parents
