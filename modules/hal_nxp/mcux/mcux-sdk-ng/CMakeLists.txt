# Copyright 2025-2026 NXP
#
# SPDX-License-Identifier: Apache-2.0

include(basic.cmake)

# Use optimized memory functions, which have better performance,
# also will avoid issues with unaligned access to peripheral RAM regions
# caused by the memcpy implementation in newlib
if(CONFIG_MCUX_OPTIMIZED_MEMCPY)
  zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk-ng/components/misc_utilities/fsl_memcpy.S)
endif()
if(CONFIG_MCUX_OPTIMIZED_MEMSET)
  zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk-ng/components/misc_utilities/fsl_memset.S)
endif()

if(CONFIG_BT_H4_NXP_CTLR AND CONFIG_BT_CLASSIC)
  # Add SCO specific configuration source file if BT Classic is enabled
  zephyr_library_sources(middleware/bt_controller/hci_nxp_sco.c)
endif()

include(middleware/middleware.cmake)
include(components/components.cmake)
include(drivers/drivers.cmake)
include(device/device.cmake)

include(fixup.cmake)
