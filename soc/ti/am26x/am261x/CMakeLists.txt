# Texas Instruments AM26x MCU Family
#
# Copyright (c) 2025 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

if(CONFIG_SOC_SERIES_AM261X_R5F)
  zephyr_sources(r5/soc.c)

  zephyr_include_directories(r5)
  zephyr_include_directories(common)

  zephyr_sources(common/ctrl_partitions.c)

  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_a_r/scripts/linker.ld CACHE INTERNAL "")

endif()
