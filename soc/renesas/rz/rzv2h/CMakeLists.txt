# Copyright (c) 2025 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

if(CONFIG_SOC_R9A09G057H44GBG_CM33)
  zephyr_include_directories(cm33)
  zephyr_sources(cm33/soc.c)
  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
endif()
