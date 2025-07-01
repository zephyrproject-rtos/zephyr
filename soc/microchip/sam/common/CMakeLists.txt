# Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_include_directories(.)

zephyr_sources(clk-generated.c)
zephyr_sources(clk-main.c)
zephyr_sources(clk-master.c)
zephyr_sources(clk-peripheral.c)
zephyr_sources(clk-programmable.c)
zephyr_sources(clk-sam9x60-pll.c)
zephyr_sources(clk-system.c)
zephyr_sources_ifdef(CONFIG_SOC_SERIES_SAMA7G5 clk-utmi.c)
zephyr_sources(pmc.c)
zephyr_sources_ifdef(CONFIG_SOC_SERIES_SAMA7D6 sama7d65.c)
zephyr_sources_ifdef(CONFIG_SOC_SERIES_SAMA7G5 sama7g5.c)
