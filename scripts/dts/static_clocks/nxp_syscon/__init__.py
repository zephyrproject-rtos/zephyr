# Copyright (c) 2026 Analog Devices
# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
from .nxp_lpc55sxx_pll import NxpLPC55SxxPLL0, NxpLPC55SxxPLL1, NxpLPC55SxxPllPdec
from .nxp_syscon_div import NxpSysconDivClockDriver
from .nxp_syscon_flexfrg import NxpSysconFlexFrgDriver
from .nxp_syscon_mux import NxpSysconClockMuxDriver
from .nxp_syscon_rtcclk import NxpSysconRtcClkDriver
from .nxp_syscon_source import NxpSysconClockSourceDriver

# zephyr-keep-sorted-stop

__all__ = [
    # zephyr-keep-sorted-start
    "NxpLPC55SxxPLL0",
    "NxpLPC55SxxPLL1",
    "NxpLPC55SxxPllPdec",
    "NxpSysconClockMuxDriver",
    "NxpSysconClockSourceDriver",
    "NxpSysconDivClockDriver",
    "NxpSysconFlexFrgDriver",
    "NxpSysconRtcClkDriver",
    # zephyr-keep-sorted-stop
]
