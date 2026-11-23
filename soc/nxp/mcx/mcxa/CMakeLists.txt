#
# Copyright 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_sources(soc.c)

zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")

zephyr_sources_ifdef(CONFIG_PM power.c)

zephyr_sources_ifdef(CONFIG_POWEROFF poweroff.c)

# Reserve the SRAM (RAM_ADDR..RAM_ADDR+0x3FFF) for the PKC engine's workareas
zephyr_linker_sources_ifdef(CONFIG_SGI_PKC RAM_SECTIONS SORT_KEY 0x0 pkc_ram_reserve.ld)
