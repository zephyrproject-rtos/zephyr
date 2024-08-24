# Copyright 2024 NXP
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(${SOC_SERIES})

zephyr_include_directories(.)
zephyr_include_directories(${SOC_SERIES})

zephyr_linker_sources_ifdef(CONFIG_NXP_IMXRT_BOOT_HEADER
  ROM_START SORT_KEY 0 boot_header.ld)

if(CONFIG_SOC_SERIES_IMXRT10XX OR CONFIG_SOC_SERIES_IMXRT11XX)
  if(CONFIG_DEVICE_CONFIGURATION_DATA)
    set(boot_hdr_dcd_data_section ".boot_hdr.dcd_data")
  endif()
  if(CONFIG_EXTERNAL_MEM_CONFIG_DATA)
    set(boot_hdr_xmcd_data_section ".boot_hdr.xmcd_data")
  endif()
  zephyr_sources(mpu_regions.c)
  zephyr_linker_section_configure(
    SECTION .rom_start
    INPUT ".boot_hdr.conf"
    OFFSET ${CONFIG_FLEXSPI_CONFIG_BLOCK_OFFSET}
    KEEP
    PRIO 10
  )
  zephyr_linker_section_configure(
    SECTION .rom_start
    INPUT ".boot_hdr.ivt"
          ".boot_hdr.data"
          ${boot_hdr_dcd_data_section}
          ${boot_hdr_xmcd_data_section}
    OFFSET ${CONFIG_IMAGE_VECTOR_TABLE_OFFSET}
    KEEP
    PRIO 11
  )
  zephyr_compile_definitions(XIP_EXTERNAL_FLASH)
endif()

if(CONFIG_SOC_SERIES_IMXRT118X)
  if(CONFIG_EXTERNAL_MEM_CONFIG_DATA)
    set(boot_hdr_xmcd_data_section ".boot_hdr.xmcd_data")
  endif()
  zephyr_linker_section_configure(
    SECTION .rom_start
    INPUT ".boot_hdr.conf"
    OFFSET ${CONFIG_FLEXSPI_CONFIG_BLOCK_OFFSET}
    KEEP
    PRIO 10
  )
  zephyr_linker_section_configure(
    SECTION .rom_start
    INPUT ${boot_hdr_xmcd_data_section}
    OFFSET ${CONFIG_EXTERNAL_MEM_CONFIG_OFFSET}
    KEEP
    PRIO 10
  )
  zephyr_linker_section_configure(
    SECTION .rom_start
    INPUT ".boot_hdr.container"
    OFFSET ${CONFIG_IMAGE_CONTAINER_OFFSET}
    KEEP
    PRIO 11
  )
  zephyr_compile_definitions(XIP_EXTERNAL_FLASH)
endif()

if(CONFIG_SOC_SERIES_IMXRT6XX OR CONFIG_SOC_SERIES_IMXRT5XX)
  zephyr_linker_sources_ifdef(CONFIG_USB_DEVICE_DRIVER SECTIONS usb.ld)
  zephyr_linker_sources_ifdef(CONFIG_UDC_DRIVER SECTIONS usb.ld)
endif()

if(CONFIG_MEMC)
  zephyr_library_include_directories(${ZEPHYR_BASE}/drivers/memc)
endif()

if(CONFIG_SOC_SERIES_IMXRT118X)
  zephyr_linker_sources_ifdef(CONFIG_SOC_SERIES_IMXRT118X SECTIONS container.ld)
endif()
