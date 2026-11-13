#
# Copyright (c) 2017, 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_library_sources(soc.c)

# The IP3511HS device controller can only access its endpoint command/status
# list and data buffers in the dedicated USB SRAM.
zephyr_linker_sources_ifdef(CONFIG_USB_DEVICE_DRIVER SECTIONS usb.ld)
zephyr_linker_sources_ifdef(CONFIG_UDC_DRIVER SECTIONS usb.ld)
zephyr_linker_sources_ifdef(CONFIG_UHC_DRIVER SECTIONS usb.ld)

zephyr_compile_definitions_ifdef(CONFIG_USB_DEVICE_DRIVER USB_STACK_USE_DEDICATED_RAM=1)
zephyr_compile_definitions_ifdef(CONFIG_UDC_DRIVER USB_STACK_USE_DEDICATED_RAM=1)
zephyr_compile_definitions_ifdef(CONFIG_UHC_DRIVER USB_STACK_USE_DEDICATED_RAM=1)
