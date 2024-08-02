# Copyright (c) 2023 TOKITA Hiroshi <tokita.hiroshi@fujitsu.com>
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_library_sources_ifdef(CONFIG_SOC_OPTION_SETTING_MEMORY
  soc.c
)

zephyr_linker_sources_ifdef(CONFIG_SOC_OPTION_SETTING_MEMORY
  ROM_START
  ${CMAKE_CURRENT_SOURCE_DIR}/opt_set_mem.ld
)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
