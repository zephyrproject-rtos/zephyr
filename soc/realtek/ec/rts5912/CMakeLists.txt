# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2024 Realtek Semiconductor Corporation, SIBG-SD7
#

zephyr_include_directories(${ZEPHYR_BASE}/drivers)
zephyr_sources(soc.c)
zephyr_include_directories(.)

zephyr_sources_ifdef(CONFIG_PM
  device_power.c
  power.c
  )

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
