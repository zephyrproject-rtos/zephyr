# Copyright 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(../../mcx/mcxw/soc.c ../../mcx/mcxw/mcxw71_platform_init.S)

zephyr_include_directories(../../mcx/mcxw/)

zephyr_sources_ifdef(
  CONFIG_PM
  power.c
)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/../../mcx/mcxw/linker.ld CACHE INTERNAL "")

zephyr_linker_sources_ifdef(CONFIG_BT RAM_SECTIONS ../../mcx/mcxw/sections.ld)
