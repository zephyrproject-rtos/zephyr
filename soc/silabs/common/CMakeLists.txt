# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc.c)

zephyr_sources_ifdef(CONFIG_SOC_GECKO_PM_BACKEND_EMU    soc_power.c)
zephyr_sources_ifdef(CONFIG_SOC_GECKO_PM_BACKEND_PMGR   soc_power_pmgr.c)

zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
