# Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_SOC_FAMILY_MICROCHIP_SAMA7 OR CONFIG_SOC_FAMILY_MICROCHIP_SAM9)
  zephyr_include_directories(.)

  zephyr_sources_ifdef(CONFIG_SOC_FAMILY_MICROCHIP_SAMA7
    clk-generated.c
    clk-main.c
    clk-master.c
    clk-peripheral.c
    clk-programmable.c
    clk-sam9x60-pll.c
    clk-system.c
    pmc.c
  )

  zephyr_sources(
    ${CONFIG_SOC_SERIES}.c
  )

  zephyr_sources_ifdef(CONFIG_SOC_SERIES_SAMA7G5
    clk-utmi.c
  )
endif()
