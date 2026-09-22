#
# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()
zephyr_library_sources(board.c)

if(CONFIG_BOARD_NXP_SPECIFIC_MPU_SETTINGS)
  if(CONFIG_SOC_MIMXRT1186_CM7)
    zephyr_library_sources(cm7/mpu_regions.c)
    # The DTCM_SECTION snippet only applies when a dedicated DTCM region is
    # chosen. On the internal-memory CM7 variant DTCM is itself zephyr,sram, so
    # no separate (overlapping) DTCM section group must be emitted.
    dt_chosen(dtcm_chosen PROPERTY "zephyr,dtcm")
    if(dtcm_chosen)
      zephyr_linker_sources(DTCM_SECTION cm7/dtcm.ld)
    endif()
  endif()
endif()

if(CONFIG_NXP_IMXRT_BOOT_HEADER)
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
