# Copyright 2023-2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_MCXW716C OR CONFIG_SOC_MCXW727C)
  zephyr_sources(mcxw7x/soc.c)
  zephyr_sources_ifdef(CONFIG_SOC_MCXW716C mcxw7x/mcxw71_platform_init.S)
  zephyr_sources_ifdef(CONFIG_SOC_MCXW727C mcxw7x/mcxw72_platform_init.S)

  zephyr_sources_ifdef(CONFIG_NXP_NBU
    ../../common/nxp_nbu.c
    )

  zephyr_include_directories(./mcxw7x)

  set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/mcxw7x/linker.ld CACHE INTERNAL "")

  if(CONFIG_BT OR CONFIG_IEEE802154)
    zephyr_linker_sources(RAM_SECTIONS /mcxw7x/sections.ld)
  endif()
endif()

if(CONFIG_SOC_MCXW236 OR CONFIG_SOC_MCXW235)
  zephyr_sources(mcxw2x/soc.c)

  # CMSIS SystemInit allows us to skip enabling clock to SRAM banks via
  # this compiler definition
  if(NOT DEFINED CONFIG_MCXW23X_SRAM_CLOCKS)
    zephyr_compile_definitions(DONT_ENABLE_DISABLED_RAMBANKS=1)
  endif()

  zephyr_include_directories(./mcxw2x)

  set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/mcxw2x/linker.ld CACHE INTERNAL "")
endif()
