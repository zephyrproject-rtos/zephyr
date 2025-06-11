# Copyright (c) 2025 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
  soc.c
  ../common/loader_program.S
)

zephyr_library_sources_ifdef(CONFIG_ARM_AARCH32_MMU mmu_regions.c)

zephyr_include_directories(.)

zephyr_linker_sources(SECTIONS sections.ld)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_a_r/scripts/linker.ld CACHE INTERNAL "")
