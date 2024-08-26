# Copyright (c) 2022-2024 MUNIC SA
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_include_directories(.)
zephyr_include_directories(../common)

zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_RA2L1
  soc.c
  ../common/ofs_mem.c
)

zephyr_linker_sources_ifdef(CONFIG_SOC_SERIES_RA2L1 ROM_START ../common/ofs_mem.ld)

set(SOC_LINKER_SCRIPT
	${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld
	CACHE INTERNAL "")
