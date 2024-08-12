# Copyright (c) 2023-2024 Analog Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(${ZEPHYR_BASE}/drivers)
zephyr_include_directories(common)
zephyr_sources(soc.c)

zephyr_linker_sources_ifdef(CONFIG_SOC_FLASH_MAX32 SECTIONS flash.ld)
zephyr_linker_sources_ifdef(CONFIG_SOC_MAX78002 SECTIONS max78002.ld)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
