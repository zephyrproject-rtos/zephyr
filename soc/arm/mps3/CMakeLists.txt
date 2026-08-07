# Copyright (c) 2021 Linaro Limited
# Copyright (c) 2026 Andrei-Edward Popa
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

if(CONFIG_SOC_AN536)
  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_a_r/scripts/linker.ld CACHE INTERNAL "")
  zephyr_library_sources_ifdef(CONFIG_ARM_MPU arm_mpu_regions.c)
else()
  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
endif()
