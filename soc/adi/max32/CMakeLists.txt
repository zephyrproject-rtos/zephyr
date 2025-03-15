# Copyright (c) 2023-2025 Analog Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(${ZEPHYR_BASE}/drivers)
zephyr_include_directories(common)
zephyr_sources(soc.c)

zephyr_linker_sources_ifdef(CONFIG_SOC_FLASH_MAX32 SECTIONS flash.ld)
if(CONFIG_SOC_MAX78000 OR CONFIG_SOC_MAX78002)
  zephyr_linker_sources(SECTIONS max7800x.ld)
endif()

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
