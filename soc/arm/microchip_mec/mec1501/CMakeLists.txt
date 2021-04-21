#
# Copyright (c) 2019, Microchip Technology Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_sources(
  soc.c
  )

zephyr_sources_ifdef(CONFIG_PM
  device_power.c
  power.c
  )

if(CONFIG_SOC_HAS_TIMING_FUNCTIONS AND NOT CONFIG_BOARD_HAS_TIMING_FUNCTIONS)
  if(CONFIG_TIMING_FUNCTIONS)
    # Use MEC15xx timing calculations only if DWT is not present
    if(NOT CONFIG_CORTEX_M_DWT)
      zephyr_library_sources(timing.c)
    endif()
  endif()
endif()
