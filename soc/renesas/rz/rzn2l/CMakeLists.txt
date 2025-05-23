# Copyright (c) 2025 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
  soc.c
  loader_param.c
  ../common/loader_program.S
)

zephyr_include_directories(.)

zephyr_linker_sources(SECTIONS sections.ld)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_r/scripts/linker.ld CACHE INTERNAL "")
