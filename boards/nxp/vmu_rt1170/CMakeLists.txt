#
# Copyright 2023 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_NXP_IMXRT_BOOT_HEADER)
  zephyr_library()
  if(CONFIG_BOOT_FLEXSPI_NOR)
    zephyr_library_sources(flexspi_nor_config.c)
    zephyr_library_include_directories(xip)
  endif()
endif()

if(CONFIG_MCUX_GPT_TIMER)
  message(WARNING "You appear to be using the GPT hardware timer. "
    "This timer will enable lower power modes, but at the cost of reduced "
    "hardware timer resolution")
endif()
