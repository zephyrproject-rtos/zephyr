# Copyright (c) 2026 Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: Apache-2.0

"""Generate the Versal NPI device-tree fragments for the Xilinx QEMU models.

The Xilinx/qemu-devicetrees project generates a handful of ".dtsi" fragments
from its Makefile (they are git-ignored build artifacts, not committed). The
Versal PMC model includes them, so they must exist before the board device
tree can be preprocessed. This script reproduces those fragments so the device
tree can be built with the C preprocessor and dtc directly, without invoking
the project's Makefile (which would force a "make" dependency on every build,
regardless of the CMake generator in use).
"""

import argparse
import os


def _guarded(macro, gen, count, semicolon):
    term = ";" if semicolon else ""
    lines = []
    for index in range(count):
        lines.append(f"#ifdef {macro}_{index}")
        lines.append(f"\tGEN_{gen}({index}){term}")
        lines.append("#endif")
    return lines


def _plain(gen, count):
    return [f"\tGEN_{gen}({index})" for index in range(count)]


def _pmc_npi_nxx():
    return (
        _guarded("MM_NPI_NOC_NMU", "NMU", 54, False)
        + _guarded("MM_NPI_NOC_NSU", "NSU", 50, False)
        + _guarded("MM_NPI_NOC_NPS", "NPS", 146, False)
        + _guarded("MM_NPI_XPIO_DCI_COMPONENT", "XPIO_DCI", 12, False)
        + _guarded("MM_NPI_CMT_XPLL", "CMT_XPLL", 25, True)
        + _guarded("MM_NPI_CMT_DPLL", "CMT_DPLL", 20, True)
        + _guarded("MM_NPI_CMT_MMCM", "CMT_MMCM", 12, True)
        + _guarded("MM_NPI_NOC_NCRB", "NOC_NCRB", 7, True)
    )


def _vp1202_pmc_npi_nxx():
    return _plain("NMU", 37) + _plain("NSU", 33) + _plain("NPS", 135)


def _gty_npi():
    return (
        _guarded("MM_NPI_GTYP_NPI_SLAVE", "GTYP", 4, True)
        + _guarded("MM_NPI_GTM_NPI_SLAVE", "GTM", 24, True)
        + _guarded("MM_NPI_GTY_NPI_SLAVE", "GTY_NPI", 10, True)
    )


FRAGMENTS = {
    "versal-pmc-npi-nxx.dtsi": _pmc_npi_nxx,
    "versal-vp1202-pmc-npi-nxx.dtsi": _vp1202_pmc_npi_nxx,
    "versal-gty-npi.dtsi": _gty_npi,
}


def main():
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument("--outdir", required=True, help="output directory")
    args = parser.parse_args()

    os.makedirs(args.outdir, exist_ok=True)
    for name, builder in FRAGMENTS.items():
        with open(os.path.join(args.outdir, name), "w") as out:
            out.write("\n".join(builder()) + "\n")


if __name__ == "__main__":
    main()
