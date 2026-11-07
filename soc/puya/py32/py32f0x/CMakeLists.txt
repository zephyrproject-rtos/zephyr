# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)
zephyr_sources(soc.c)

zephyr_compile_definitions(HSI_VALUE=24000000U)

set(SOC_LINKER_SCRIPT
  ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld
  CACHE INTERNAL ""
)
