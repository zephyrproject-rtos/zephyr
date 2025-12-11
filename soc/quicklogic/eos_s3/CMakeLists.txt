# Copyright (c) 2020 Antmicro <www.antmicro.com>
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
  soc.c
  )

zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
