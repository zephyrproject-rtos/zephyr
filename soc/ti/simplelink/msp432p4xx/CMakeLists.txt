# SPDX-License-Identifier: Apache-2.0

zephyr_compile_definitions(-D__MSP432P401R__)
zephyr_sources(soc.c)
zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
