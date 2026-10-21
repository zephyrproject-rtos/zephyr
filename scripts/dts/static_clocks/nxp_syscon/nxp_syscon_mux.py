# Copyright (c) 2026 Analog Devices
# SPDX-License-Identifier: Apache-2.0

from common import ClockDriver


class NxpSysconClockMuxDriver(ClockDriver):
    """
    Clock driver for nxp,syscon-clock-mux nodes.
    Specifier 'multiplexer': zero-based index into the input-sources
    phandle array selecting which parent clock to use.
    """

    @classmethod
    def get_dt_compat(cls):
        return "nxp,syscon-clock-mux"

    def __init__(self, dt_node):
        super().__init__(dt_node)
        self.parent_count = len(dt_node.props['input-sources'].val)
        self.parents = dt_node.props['input-sources'].val

    def default_specifiers(self):
        # Assumption: mux defaults to index 0 at reset.
        return {'multiplexer': 0}

    def mux_recalc(self, specifiers: dict[str, int]):
        idx = specifiers['multiplexer']
        if idx < 0 or idx >= self.parent_count:
            return -1
        return idx

    def get_parents(self):
        return self.parents
