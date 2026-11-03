#
# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0

"""
i.MX RT266x SoC configuration for the nxp,imx-ccm-rev3 clock-id generator.

RT266x follows the rev3-family enum naming conventions with no exceptions, so
this config is just the default name-extraction rules. A future SoC whose SDK
deviates would override the methods below.
"""

import re


class Rt266xConfig:
    soc = "rt266x"
    display = "RT266x"

    # SoC-specific note folded into the generated header's file comment.
    id_space_doc = (
        " - @c IMX_CCM_LPCG_* -- peripheral clock gates. The RT266x clock tree spans\n"
        "   seven CCM instances (SYSCON, CMPT, MAIN, WAKE, COMM, AUDIO, MEDIA); a gate\n"
        "   identifier names both the owning instance and the slice within it, which\n"
        "   the HAL resolves at run time.\n"
        " - @c IMX_CCM_ROOT_* -- clock roots, likewise spanning all seven instances.\n"
        "   A root is what carries a mux and dividers, so it is what determines a\n"
        "   peripheral's frequency.\n"
        " - @c IMX_CCM_MUX_<root>_<source> -- the per-root mux selector values. Each\n"
        "   root has its own source list, so these are namespaced by root."
    )

    def lpcg_tail(self, enumerator):
        # kCLOCK_<TAIL> -> <TAIL>
        return enumerator[len("kCLOCK_") :]

    def root_tail(self, enumerator):
        # kCLOCK_Root_<TAIL> -> <TAIL>
        return re.sub(r"^Root_", "", enumerator[len("kCLOCK_") :])

    def mux_name(self, enumerator):
        # kCLOCK_<ROOT>_ClockRoot_<SOURCE> -> <ROOT>_<SOURCE>; None if not a mux entry
        m = re.match(r"kCLOCK_(.+?)_ClockRoot_(.+)$", enumerator)
        return f"{m.group(1)}_{m.group(2)}" if m else None


CONFIG = Rt266xConfig()
