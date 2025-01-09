#
# Copyright 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_sources(soc.c)
zephyr_sources_ifdef(CONFIG_PM power.c)

zephyr_include_directories(.)

if(CONFIG_MEMC_MCUX_FLEXSPI)
  zephyr_sources(flexspi.c)
  if(CONFIG_FLASH_MCUX_FLEXSPI_XIP)
    zephyr_code_relocate(FILES flexspi.c LOCATION ${CONFIG_FLASH_MCUX_FLEXSPI_XIP_MEM}_TEXT)
  endif()
endif()

if(CONFIG_SECOND_CORE_MCUX_REMOTE_DIR)
  # Add remote directory to include within the build
  zephyr_include_directories(${CONFIG_SECOND_CORE_MCUX_REMOTE_DIR})
endif()


set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")
