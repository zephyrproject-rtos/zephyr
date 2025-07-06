#
# Copyright 2024-2025 NXP
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

if((CONFIG_NXP_IMXRT_BOOT_HEADER) AND (CONFIG_BOOT_FLEXSPI_NOR))
  set(RT118x_DEVICE_BOOT_HEADER_DIR
    "${ZEPHYR_HAL_NXP_MODULE_DIR}/mcux/mcux-sdk-ng/devices/RT/RT1180/MIMXRT1189")

  zephyr_library_sources(${RT118x_DEVICE_BOOT_HEADER_DIR}/xip/fsl_flexspi_nor_boot.c)
  zephyr_library_include_directories(${RT118x_DEVICE_BOOT_HEADER_DIR}/xip)
  zephyr_library_include_directories(${RT118x_DEVICE_BOOT_HEADER_DIR})
endif()

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")
