# Copyright (c) 2020, MADMACHINE LIMITED
#
# SPDX-License-Identifier: Apache-2.0

import os

def will_connect():
    # Look up the external flash memory region.

    # 'target' is global
    # pylint: disable=undefined-variable
    extFlash = target.memory_map.get_region_by_name("flexspi")

    # Set the path to an .FLM flash algorithm.
    flm_path = os.getenv('ZEPHYR_BASE') + '/boards/arm/mm_swiftio/burner/MIMXRT105x_QuadSPI_4KB_SEC.FLM'
    extFlash.flm = flm_path
