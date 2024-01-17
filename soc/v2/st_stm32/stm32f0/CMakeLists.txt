# SPDX-License-Identifier: Apache-2.0

# The vector table must be placed at the start of SRAM
zephyr_linker_sources_ifdef(CONFIG_SRAM_VECTOR_TABLE
  RAM_SECTIONS
  SORT_KEY 0
  sram_vector_table.ld
)
zephyr_include_directories(${ZEPHYR_BASE}/drivers)
zephyr_sources(soc.c)

zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
