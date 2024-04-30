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
if(DEFINED CONFIG_LPC55XXX_USB_RAM)
zephyr_linker_sources_ifdef(CONFIG_USB_DEVICE_DRIVER
  SECTIONS usb.ld)

zephyr_compile_definitions_ifdef(CONFIG_USB_DEVICE_DRIVER USB_STACK_USE_DEDICATED_RAM=1)
endif()

# CMSIS SystemInit allows us to skip enabling clock to SRAM banks via
# this compiler definition
if(NOT DEFINED CONFIG_LPC55XXX_SRAM_CLOCKS)
zephyr_compile_definitions(DONT_ENABLE_DISABLED_RAMBANKS=1)
endif()

if (CONFIG_SOC_LPC55S69_CPU0)
  zephyr_sources_ifdef(CONFIG_PM power.c)
endif()

zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")
