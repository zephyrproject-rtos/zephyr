# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_SOC_MKW24D5        soc_kw2xd.c)
zephyr_sources_ifdef(CONFIG_SOC_MKW22D5        soc_kw2xd.c)
zephyr_sources_ifdef(CONFIG_SOC_MKW41Z4        soc_kw4xz.c)
zephyr_sources_ifdef(CONFIG_SOC_MKW40Z4        soc_kw4xz.c)

zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
