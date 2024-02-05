# Copyright (c) 2022 Vaishnav Achath
#
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc.c)
zephyr_sources(ccfg.c)

zephyr_include_directories(.)

zephyr_library_sources_ifdef(CONFIG_PM    power.c)
zephyr_library_sources_ifdef(CONFIG_PM_DEVICE   power.c)

zephyr_linker_sources_ifdef(CONFIG_HAS_TI_CCFG SECTIONS ccfg.ld)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
