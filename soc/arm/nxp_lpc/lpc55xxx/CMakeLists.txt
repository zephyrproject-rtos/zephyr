#
# Copyright (c) 2019, NXP
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources(soc.c)

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/${ARCH}/include
  )

zephyr_linker_sources_ifdef(CONFIG_USB_DEVICE_DRIVER
  SECTIONS usb.ld)

zephyr_compile_definitions_ifdef(CONFIG_USB_DEVICE_DRIVER USB_STACK_USE_DEDICATED_RAM=1)
