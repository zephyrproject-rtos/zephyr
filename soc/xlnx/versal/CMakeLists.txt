#
# Copyright (c) 2025 Space Cubics Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_sources(soc.c)

zephyr_include_directories(.)

if(CONFIG_SOC_VERSAL_RPU)
  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_a_r/scripts/linker.ld CACHE INTERNAL "")
endif()
