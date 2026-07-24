# Copyright (c) 2026 Linumiz
# Copyright (c) 2026 Infineon Technologies AG
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

if(CONFIG_CPU_CORTEX_M0PLUS)
  add_subdirectory(m0p)
endif()

if(CONFIG_CPU_CORTEX_M7)
  add_subdirectory(m7)
endif()

zephyr_sources(soc_vector_table.c)

zephyr_linker_sources(ROM_START SORT_KEY 0x0vectors ld/vector_table.ld)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
