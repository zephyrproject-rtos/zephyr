# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_FAMILY_SILABS_S0 OR CONFIG_SOC_FAMILY_SILABS_S1)
  zephyr_sources(soc.c)
endif()

if((CONFIG_SOC_FAMILY_SILABS_S1 AND CONFIG_SILABS_GECKO_POWER_MANAGER) OR CONFIG_SOC_FAMILY_SILABS_S2)
  zephyr_sources_ifdef(CONFIG_PM soc_power_pmgr.c)
else()
  zephyr_sources_ifdef(CONFIG_PM soc_power.c)
endif()

zephyr_sources_ifdef(CONFIG_DT_HAS_SILABS_SERIES0_CMU_ENABLED clock.c)
zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
