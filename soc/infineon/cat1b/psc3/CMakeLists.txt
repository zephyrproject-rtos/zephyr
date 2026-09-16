# Copyright (c) 2025 Cypress Semiconductor Corporation.
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc.c)
zephyr_include_directories(.)
zephyr_linker_sources(NOINIT noinit.ld)

zephyr_sources_ifdef(CONFIG_PM power.c)

# Skip the BSP's deep-sleep-RAM warmboot code: it needs the MTB startup file
# and linker script that Zephyr replaces.
zephyr_compile_definitions(CYBSP_SKIP_PM_REGISTRATION)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
