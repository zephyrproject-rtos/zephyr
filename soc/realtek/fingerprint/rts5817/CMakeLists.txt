# Copyright (c) 2025 Realtek Semiconductor, Inc.
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_sources(soc.c)

zephyr_sources_ifdef(CONFIG_PM soc_power.c)
zephyr_sources_ifdef(CONFIG_PM_S2RAM soc_pm_s2ram.c)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
