# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(${ZEPHYR_BASE}/drivers)

zephyr_sources_ifdef(CONFIG_CPU_CORTEX_M7 soc_m7.c)
zephyr_sources_ifdef(CONFIG_CPU_CORTEX_M4 soc_m4.c)

zephyr_sources(mpu_regions.c)
zephyr_linker_sources(SECTIONS sections.ld)

zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
