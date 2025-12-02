# Copyright 2022-2024 NXP
#
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
  soc.c
  flexspi_clock_setup.c
  )

zephyr_sources_ifdef(CONFIG_NXP_NBU
  ../common/nxp_nbu.c
  )

zephyr_sources_ifdef(CONFIG_PM
  power.c
  )

zephyr_linker_sources_ifdef(CONFIG_NXP_RW6XX_BOOT_HEADER
  ROM_START SORT_KEY 0 boot_header.ld)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")

zephyr_include_directories(.)
zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../common)

zephyr_linker_sources(RODATA firmwares.ld)

zephyr_linker_sources(RAM_SECTIONS sections.ld)
