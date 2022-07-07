# SPDX-License-Identifier: Apache-2.0

if (CONFIG_SD_STACK)
  zephyr_interface_library_named(SD)

  zephyr_library()
  zephyr_library_sources (sd.c sdmmc.c)
endif()
