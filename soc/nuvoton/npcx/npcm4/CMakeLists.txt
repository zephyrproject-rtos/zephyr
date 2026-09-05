#
# Copyright (c) 2024 Nuvoton Technology Corporation.
#
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(
  .
  ${ZEPHYR_BASE}/drivers
)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
