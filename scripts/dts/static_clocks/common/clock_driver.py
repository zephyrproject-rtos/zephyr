# Copyright (c) 2026 Analog Devices
# SPDX-License-Identifier: Apache-2.0


class ClockDriver:
    """
    Abstract class for a clock driver. SOCs should implement this subclass
    to support build time validation and rate calculation.
    """

    @classmethod
    def get_dt_compat(cls):
        """
        Get the device tree compatible string for this clock driver.
        """

    def __init__(self, dt_node):
        """
        Initialize the clock driver with the given device tree node.
        @param dt_node: The EDTLib node to initialize the clock driver with.
        """
        self.name = dt_node.name

    def configure_recalc(self, specifiers: list[int], parent_rate: int):
        """
        recalculate the clock rate based on the given specifiers and parent rate.
        @param specifiers: the specifiers used to configure the clock.
        @param parent_rate: the rate of the parent clock.
        @return: the recalculated clock rate, or a negative value if the specifier is invalid.
        """

    def default_specifiers(self):
        """
        Get the default specifiers for this clock node. Ususally the
        reset value of the fields being configured.
        """

    def mux_recalc(self, specifiers: dict[str, int]):
        """
        For multiplexer clock nodes: validate the mux specifier and return
        the selected parent index.
        @param specifiers: the specifiers used to configure the mux.
        @return: the selected parent index (zero-based), or a negative
                 value if the specifier is invalid.
        """

    def root_recalc(self, specifiers: dict[str, int]):
        """
        For root clock nodes (no parent): recalculate the output rate
        based on the given specifiers alone.
        @param specifiers: the specifiers used to configure the root clock.
        @return: the output clock rate in Hz, or a negative value if
                 the specifier is invalid.
        """

    def get_parents(self):
        """
        Get the list of parent clocks for this clock node.
        @return: a list of parent clock nodes. This should be empty for
        root clocks, a single element for standard clocks, and
        multiple elements for muxed clocks.
        """

    def __str__(self):
        return f"<{self.__class__.__name__} name={self.name}>"
