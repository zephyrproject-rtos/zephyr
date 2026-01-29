# Copyright (c) 2019 STMicroelectronics
#
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(${ZEPHYR_BASE}/drivers)
zephyr_sources(
  soc.c
  )

zephyr_include_directories(.)

zephyr_linker_sources_ifdef(CONFIG_OPENAMP_RSC_TABLE ROM_SECTIONS rsc_table.ld)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
