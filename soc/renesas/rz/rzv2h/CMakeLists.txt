# Copyright (c) 2025 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

if(CONFIG_SOC_R9A09G057H44GBG_CM33)
  zephyr_include_directories(cm33)
  zephyr_sources(cm33/soc.c)

  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")

elseif(CONFIG_SOC_R9A09G057H44GBG_CR8)
  zephyr_include_directories(cr8)
  zephyr_sources(cr8/soc.c)

  zephyr_sources_ifdef(CONFIG_ARM_MPU cr8/arm_mpu_regions.c)

  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_a_r/scripts/linker.ld CACHE INTERNAL "")
endif()
