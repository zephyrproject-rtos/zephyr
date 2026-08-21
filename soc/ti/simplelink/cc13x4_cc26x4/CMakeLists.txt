# Copyright (c) 2026 Texas Instruments
#
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc.c)
zephyr_sources(ccfg.c)

zephyr_include_directories(.)
zephyr_library_sources_ifdef(CONFIG_PM    power.c)
zephyr_library_sources_ifdef(CONFIG_POWEROFF poweroff.c)

zephyr_linker_sources_ifdef(CONFIG_HAS_TI_CCFG SECTIONS ccfg.ld)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
