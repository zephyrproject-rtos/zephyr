#
# Copyright (c) 2025, Microchip Technology Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_include_directories(${ZEPHYR_BASE}/drivers)
zephyr_sources(soc.c)
zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
