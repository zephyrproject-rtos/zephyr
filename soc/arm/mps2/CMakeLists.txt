# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc.c)
if(CONFIG_MP_MAX_NUM_CPUS GREATER 1)
  zephyr_sources(smp.c)
endif()

zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
