# Copyright(c) 2026, Realtek Semiconductor Corporation.
# SPDX-License-Identifier: Apache-2.0

# add header files for zephyr implementations
zephyr_include_directories(inc)

if(CONFIG_SOC_SERIES_RTL87X2G)
  # add osif header files
  zephyr_include_directories(${ZEPHYR_HAL_REALTEK_MODULE_DIR}/bee/os/osif/inc/rtl87x2g)

  zephyr_library_sources(src/rtl87x2g/osif_zephyr.c)
endif()
