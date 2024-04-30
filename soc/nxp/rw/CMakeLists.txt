# Copyright 2022-2024 NXP
#
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
  soc.c
  flexspi_clock_setup.c
  )

zephyr_linker_sources_ifdef(CONFIG_NXP_RW6XX_BOOT_HEADER
  ROM_START SORT_KEY 0 boot_header.ld)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
