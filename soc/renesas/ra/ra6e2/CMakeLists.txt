# Copyright (c) 2024 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_sources(
  soc.c
)

zephyr_linker_sources(SECTIONS sections.ld)
zephyr_linker_sources(RAM_SECTIONS ram_sections.ld)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
