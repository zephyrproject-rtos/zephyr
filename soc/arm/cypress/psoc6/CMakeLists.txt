#
# Copyright (c) 2018, Cypress
# Copyright (c) 2020, ATL Electronics
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_include_directories(.)
zephyr_sources(
	soc.c
)

zephyr_linker_sources_ifdef(CONFIG_SOC_FAMILY_PSOC6 NOINIT noinit.ld)
zephyr_linker_sources_ifdef(CONFIG_SOC_FAMILY_PSOC6 RWDATA rwdata.ld)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
