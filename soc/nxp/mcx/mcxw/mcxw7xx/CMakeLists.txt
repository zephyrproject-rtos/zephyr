# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc.c)
zephyr_sources_ifdef(CONFIG_SOC_MCXW716C mcxw71_platform_init.S)
zephyr_sources_ifdef(CONFIG_SOC_MCXW727C mcxw72_platform_init.S)

zephyr_sources_ifdef(CONFIG_NXP_NBU
  ../../../common/nxp_nbu.c
  )

zephyr_include_directories(./)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")

if(CONFIG_BT OR CONFIG_IEEE802154)
  zephyr_linker_sources(RAM_SECTIONS /sections.ld)
endif()
