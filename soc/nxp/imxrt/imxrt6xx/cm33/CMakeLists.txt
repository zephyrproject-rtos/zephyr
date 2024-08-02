#
# Copyright (c) 2020, NXP
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_include_directories(.)

zephyr_sources(soc.c)

zephyr_sources_ifdef(CONFIG_PM power.c)

zephyr_sources_ifdef(CONFIG_FLASH_MCUX_FLEXSPI_XIP flash_clock_setup.c)

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/${ARCH}/include
  )

zephyr_compile_definitions_ifdef(CONFIG_USB_DEVICE_DRIVER USB_STACK_USE_DEDICATED_RAM=1)
zephyr_compile_definitions_ifdef(CONFIG_UDC_DRIVER USB_STACK_USE_DEDICATED_RAM=1)

if(CONFIG_FLASH_MCUX_FLEXSPI_XIP)
  zephyr_code_relocate(FILES flash_clock_setup.c LOCATION RAM)
endif()

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
