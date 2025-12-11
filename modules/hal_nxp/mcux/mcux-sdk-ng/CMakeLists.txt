# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

include(basic.cmake)

if((${MCUX_DEVICE} MATCHES "RW61") AND (NOT DEFINED CONFIG_MINIMAL_LIBC))
  # Whenever building for RW61x without minimal LIBC, use optimized memcpy.
  # This will avoid issues with unaligned access to peripheral RAM regions
  # caused by the memcpy implementation in newlib
  zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk-ng/components/misc_utilities/fsl_memcpy.S)
endif()

include(middleware/middleware.cmake)
include(components/components.cmake)
include(drivers/drivers.cmake)
include(device/device.cmake)

include(fixup.cmake)
