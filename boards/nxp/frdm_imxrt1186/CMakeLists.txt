#
# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()

if(CONFIG_NXP_BOARD_SPECIFIC_MPU_SETTINGS)
  if(CONFIG_SOC_MIMXRT1186_CM7)
    zephyr_sources(cm7/mpu_regions.c)
    zephyr_linker_sources(DTCM_SECTION cm7/dtcm.ld)
  endif()
endif()

if(CONFIG_NXP_IMXRT_BOOT_HEADER)
  if(NOT ((DEFINED CONFIG_BOARD_FRDM_IMXRT1186_MIMXRT1186_CM33)
    OR (DEFINED CONFIG_BOARD_FRDM_IMXRT1186_MIMXRT1186_CM7)))
    message(WARNING "It appears you are using the board definition for "
      "the FRDM-IMXRT1186, but targeting a custom board. You may need to "
      "update your flash configuration or device configuration data blocks")
  endif()
  if(CONFIG_BOOT_FLEXSPI_NOR)
    # This flash configuration block may need modification if another
    # flash chip is used on your custom board.
    zephyr_library_compile_definitions(XIP_EXTERNAL_FLASH=1)
    zephyr_library_sources(xip/frdmimxrt1186_flexspi_nor_config.c)
    zephyr_library_include_directories(xip)
  endif()
  if(CONFIG_EXTERNAL_MEM_CONFIG_DATA AND CONFIG_NXP_IMX_EXTERNAL_HYPERRAM)
    zephyr_library_compile_definitions(USE_HYPERRAM)
    zephyr_library_sources(xip/frdmimxrt1186_flexspi_nor_config.c)
  endif()
endif()
