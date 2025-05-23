# Copyright (c) 2021-2025 ATL Electronics
#
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)
zephyr_sources(soc.c)

zephyr_linker_sources_ifdef(CONFIG_SOC_SERIES_BL60X RODATA rodata.ld)

set(SOC_LINKER_SCRIPT
  ${ZEPHYR_BASE}/include/zephyr/arch/riscv/common/linker.ld
  CACHE INTERNAL ""
  )
