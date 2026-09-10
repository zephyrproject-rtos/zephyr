# Copyright (c) 2018 Foundries.io Ltd
#
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
  vector.S
  soc_irq.S
  wdog.S
  soc.c
)

zephyr_include_directories(.)

zephyr_linker_sources(ROM_START SORT_KEY 0x0vectors vector_table.ld)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")
