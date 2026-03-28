# Copyright 2025-2026 NXP
#
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc.c)
zephyr_sources_ifdef(CONFIG_SOC_MCXW70AC mcxw70_platform_init.S)
zephyr_sources_ifdef(CONFIG_SOC_MCXW716C mcxw71_platform_init.S)
zephyr_sources_ifdef(CONFIG_SOC_MCXW727C mcxw72_platform_init.S)

zephyr_sources_ifdef(CONFIG_PM power.c)

zephyr_include_directories(./)

zephyr_linker_sources_ifdef(CONFIG_NXP_MCXW7XX_BOOT_HEADER
  ROM_START SORT_KEY 0 boot_header.ld)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")

if(CONFIG_NXP_NBU)
  zephyr_linker_sources(RAM_SECTIONS sections.ld)
endif()
