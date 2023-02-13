#
# Copyright (c) 2022, NXP
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_compile_definitions(FSL_SDK_DRIVER_QUICK_ACCESS_ENABLE)

zephyr_compile_definitions_ifdef(CONFIG_USB_DEVICE_DRIVER USB_STACK_USE_DEDICATED_RAM=1)

zephyr_sources(
  soc.c
  flash_clock_setup.c
  )

zephyr_sources_ifdef(CONFIG_PM power.c)

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/${ARCH}/include
  )

zephyr_linker_sources_ifdef(CONFIG_NXP_IMX_RT5XX_BOOT_HEADER
  ROM_START SORT_KEY 0 boot_header.ld)

zephyr_linker_sources_ifdef(CONFIG_USB_DEVICE_DRIVER
  SECTIONS usb.ld)

zephyr_code_relocate(FILES flash_clock_setup.c LOCATION RAM)
