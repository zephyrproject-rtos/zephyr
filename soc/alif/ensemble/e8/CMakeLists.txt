# Copyright (c) 2025 Alif Semiconductor
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_FAMILY_ENSEMBLE_RTSS)
  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
endif()

if(CONFIG_SOC_FAMILY_ENSEMBLE_APSS)
  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_a_r/scripts/linker.ld CACHE INTERNAL "")
endif()
