# Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_SOC_FAMILY_MICROCHIP_SAMA7)
  zephyr_include_directories(.)

  zephyr_sources(clk-generated.c
    clk-main.c
    clk-master.c
    clk-peripheral.c
    clk-programmable.c
    clk-sam9x60-pll.c
    clk-system.c
    pmc.c
  )
  zephyr_sources_ifdef(CONFIG_SOC_SERIES_SAMA7G5
    clk-utmi.c
    sama7g5.c
  )
  zephyr_sources_ifdef(CONFIG_SOC_SERIES_SAMA7D6 sama7d65.c)
endif()
