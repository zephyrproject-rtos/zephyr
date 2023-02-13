#
# Copyright (c) 2020, NXP
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_sources(
  soc.c
  )

zephyr_sources_ifdef(CONFIG_PM
  power.c
  )

zephyr_sources_ifdef(CONFIG_FLASH_MCUX_FLEXSPI_XIP
  flash_clock_setup.c
  )

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/${ARCH}/include
  )

zephyr_compile_definitions_ifdef(CONFIG_USB_DEVICE_DRIVER USB_STACK_USE_DEDICATED_RAM=1)

zephyr_linker_sources_ifdef(CONFIG_NXP_IMX_RT6XX_BOOT_HEADER
  ROM_START SORT_KEY 0 boot_header.ld)

zephyr_linker_sources_ifdef(CONFIG_USB_DEVICE_DRIVER
  SECTIONS usb.ld)

if(CONFIG_FLASH_MCUX_FLEXSPI_XIP)
  zephyr_code_relocate(FILES flash_clock_setup.c LOCATION RAM)
endif()
