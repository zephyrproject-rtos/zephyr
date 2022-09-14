# Copyright (c) 2019 Brett Witherspoon
#
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc.c)
zephyr_sources(ccfg.c)
zephyr_include_directories(.)

if(CONFIG_PM OR CONFIG_PM_DEVICE OR CONFIG_POWEROFF)
  zephyr_library_sources(power.c)
endif()
zephyr_library_sources_ifdef(CONFIG_POWEROFF poweroff.c)

zephyr_linker_sources_ifdef(CONFIG_HAS_TI_CCFG SECTIONS ccfg.ld)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
