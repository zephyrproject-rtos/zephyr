#
# Copyright (c) 2021, Microchip Technology Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_include_directories(${ZEPHYR_BASE}/drivers)
zephyr_sources(
  soc.c
  )

if(CONFIG_PM)
  zephyr_library_sources(power.c device_power.c)
endif()

if(CONFIG_SOC_HAS_TIMING_FUNCTIONS AND NOT CONFIG_BOARD_HAS_TIMING_FUNCTIONS)
  if(CONFIG_TIMING_FUNCTIONS)
    # Use MEC172x timing calculations only if DWT is not present
    if(NOT CONFIG_CORTEX_M_DWT)
      zephyr_library_sources(timing.c)
    endif()
  endif()
endif()
