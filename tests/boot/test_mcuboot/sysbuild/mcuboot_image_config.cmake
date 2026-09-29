# Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

include(${ZEPHYR_BASE}/share/sysbuild/image_configurations/BOOTLOADER_image_default.cmake)

if(BOARD STREQUAL "frdm_mcxn947" AND
   BOARD_QUALIFIERS STREQUAL "mcxn947/cpu0")
  set_config_int(${ZCMAKE_APPLICATION} CONFIG_MCUBOOT_LOGICAL_SECTOR_SIZE 0x2000)
endif()
