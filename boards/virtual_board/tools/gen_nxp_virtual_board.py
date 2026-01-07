#!/usr/bin/env python3
#
# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

"""Entry point for the NXP virtual board generator.

The implementation lives under:
  boards/nxp/virtual_boards/nxp/

This wrapper is kept to preserve existing documentation/CI invocations.
"""

# Optional configuration:
# - include_part_number_variants: if true, expand each SoC target into multiple
#   targets keyed by SOC_PART_NUMBER_* symbols discovered in soc/**/Kconfig.soc.
#   This generates qualifiers like:
#     nxp_virtual/mimxrt1189/cm33/MIMXRT1187CVM8C
#   and defconfigs that explicitly set:
#     CONFIG_SOC_PART_NUMBER_MIMXRT1187CVM8C=y
#
# Default: false (keeps the historical "one target per SoC[/cpucluster]" behavior).

from __future__ import annotations

import sys
from pathlib import Path


def _import_impl():
    zephyr_root = Path(__file__).resolve().parents[3]
    impl_dir = zephyr_root / "boards" / "nxp" / "virtual_boards" / "nxp"
    common_dir = zephyr_root / "boards" / "virtual_board" / "tools"

    # Ensure shared helper module can be imported by the implementation.
    sys.path.insert(0, str(common_dir))
    sys.path.insert(0, str(impl_dir))
    import gen_nxp_virtual_board_impl as impl  # type: ignore

    return impl


def main() -> int:
    impl = _import_impl()
    return int(impl.main())


if __name__ == "__main__":
    raise SystemExit(main())
