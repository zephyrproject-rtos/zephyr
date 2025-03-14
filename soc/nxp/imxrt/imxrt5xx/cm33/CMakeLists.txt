#
# Copyright 2022,2024 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_compile_definitions(FSL_SDK_DRIVER_QUICK_ACCESS_ENABLE)

zephyr_compile_definitions_ifdef(CONFIG_USB_DEVICE_DRIVER USB_STACK_USE_DEDICATED_RAM=1)
zephyr_compile_definitions_ifdef(CONFIG_UDC_DRIVER USB_STACK_USE_DEDICATED_RAM=1)

zephyr_include_directories(.)

zephyr_sources(soc.c)

zephyr_sources_ifdef(CONFIG_FLASH_MCUX_FLEXSPI_XIP flash_clock_setup.c)

zephyr_sources_ifdef(CONFIG_PM power.c)

zephyr_sources_ifdef(CONFIG_POWEROFF poweroff.c)

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/${ARCH}/include
  )

zephyr_code_relocate(FILES flash_clock_setup.c LOCATION RAM)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")
