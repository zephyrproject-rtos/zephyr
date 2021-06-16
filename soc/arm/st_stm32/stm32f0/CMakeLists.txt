# SPDX-License-Identifier: Apache-2.0

# The vector table must be placed at the start of SRAM
zephyr_linker_sources_ifdef(CONFIG_SRAM_VECTOR_TABLE
  RAM_SECTIONS
  SORT_KEY 0
  sram_vector_table.ld
)
zephyr_include_directories(${ZEPHYR_BASE}/drivers)
zephyr_sources(soc.c)
