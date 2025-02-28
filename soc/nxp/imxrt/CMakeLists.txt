# Copyright 2024 NXP
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(${SOC_SERIES})

zephyr_include_directories(.)
zephyr_include_directories(${SOC_SERIES})

zephyr_linker_sources_ifdef(CONFIG_NXP_IMXRT_BOOT_HEADER
  ROM_START SORT_KEY 0 boot_header.ld)

if(CONFIG_SOC_SERIES_IMXRT10XX OR CONFIG_SOC_SERIES_IMXRT11XX)
  zephyr_sources_ifdef(CONFIG_ARM_MPU mpu_regions.c)
endif()

if(CONFIG_SOC_SERIES_IMXRT10XX OR CONFIG_SOC_SERIES_IMXRT11XX OR CONFIG_SOC_SERIES_IMXRT118X)
  zephyr_compile_definitions(XIP_EXTERNAL_FLASH)
endif()

if(CONFIG_SOC_SERIES_IMXRT6XX OR CONFIG_SOC_SERIES_IMXRT5XX OR CONFIG_SOC_SERIES_IMXRT7XX)
  zephyr_linker_sources_ifdef(CONFIG_USB_DEVICE_DRIVER SECTIONS usb.ld)
  zephyr_linker_sources_ifdef(CONFIG_UDC_DRIVER SECTIONS usb.ld)
endif()

if(CONFIG_MEMC)
  zephyr_library_include_directories(${ZEPHYR_BASE}/drivers/memc)
endif()

if(CONFIG_SOC_SERIES_IMXRT118X)
  zephyr_linker_sources_ifdef(CONFIG_SOC_SERIES_IMXRT118X SECTIONS container.ld)
endif()
