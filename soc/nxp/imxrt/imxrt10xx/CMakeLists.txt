#
# Copyright 2024-2025 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()
zephyr_library_sources(soc.c)

if(CONFIG_PM)
  zephyr_sources(power.c)
  zephyr_code_relocate(FILES power.c LOCATION ITCM_TEXT)
  if(CONFIG_SOC_MIMXRT1064)
    zephyr_sources(lpm_rt1064.c)
    zephyr_code_relocate(FILES lpm_rt1064.c LOCATION ITCM_TEXT)
  endif()
endif()

if(CONFIG_MEMC_MCUX_FLEXSPI)
  zephyr_sources(flexspi.c)
  if(CONFIG_FLASH_MCUX_FLEXSPI_XIP)
    zephyr_code_relocate(FILES flexspi.c LOCATION ${CONFIG_FLASH_MCUX_FLEXSPI_XIP_MEM}_TEXT)
  endif()
endif()

if(CONFIG_NXP_IMXRT_BOOT_HEADER)
  if(CONFIG_BOOT_FLEXSPI_NOR)
    zephyr_library_compile_definitions(BOARD_FLASH_SIZE=CONFIG_FLASH_SIZE*1024)
  endif()

  if(CONFIG_DEVICE_CONFIGURATION_DATA)
    zephyr_library_compile_definitions(XIP_BOOT_HEADER_DCD_ENABLE=1)
  endif()
endif()

zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")
