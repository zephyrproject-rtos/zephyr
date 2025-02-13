#
# Copyright 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_sources(soc.c)

if(CONFIG_SOC_MIMXRT1189_CM33)
  zephyr_sources(m33/mpu_regions.c)
  zephyr_linker_sources(DTCM_SECTION m33/dtcm.ld)
endif()

if(CONFIG_SOC_MIMXRT1189_CM7)
  zephyr_sources(m7/mpu_regions.c)
endif()

zephyr_include_directories(.)

if(CONFIG_MEMC_MCUX_FLEXSPI)
  zephyr_sources(flexspi.c)
  if(CONFIG_FLASH_MCUX_FLEXSPI_XIP)
    zephyr_code_relocate(FILES flexspi.c LOCATION ${CONFIG_FLASH_MCUX_FLEXSPI_XIP_MEM}_TEXT)
  endif()
endif()

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")
