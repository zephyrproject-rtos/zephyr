#
# Copyright (c) 2019,2024 NXP
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
  zephyr_linker_sources_ifdef(CONFIG_UDC_DRIVER
  SECTIONS usb.ld)
  zephyr_linker_sources_ifdef(CONFIG_UHC_DRIVER
  SECTIONS usb.ld)

if(CONFIG_USB_DEVICE_DRIVER OR CONFIG_UDC_DRIVER OR CONFIG_UHC_DRIVER)
  zephyr_linker_group(NAME USB_BDT VMA USB_SRAM)

  zephyr_linker_section(
    NAME  _USB_BDT_SECTION_NAME
    GROUP USB_BDT
    TYPE  NOLOAD
  )

  zephyr_linker_section_configure(
    SECTION _USB_BDT_SECTION_NAME
    INPUT "m_usb_bdt"
    ALIGN 512
  )

  zephyr_linker_group(NAME USB_GLOBAL VMA USB_SRAM)

  zephyr_linker_section(
    NAME  _USB_GLOBAL_SECTION_NAME
    GROUP USB_GLOBAL
    TYPE  NOLOAD
  )

  zephyr_linker_section_configure(
    SECTION _USB_GLOBAL_SECTION_NAME
    INPUT "m_usb_global"
  )
endif()

zephyr_compile_definitions_ifdef(CONFIG_USB_DEVICE_DRIVER USB_STACK_USE_DEDICATED_RAM=1)
zephyr_compile_definitions_ifdef(CONFIG_UDC_DRIVER USB_STACK_USE_DEDICATED_RAM=1)
zephyr_compile_definitions_ifdef(CONFIG_UHC_DRIVER USB_STACK_USE_DEDICATED_RAM=1)
endif()

# CMSIS SystemInit allows us to skip enabling clock to SRAM banks via
# this compiler definition
if(NOT DEFINED CONFIG_LPC55XXX_SRAM_CLOCKS)
zephyr_compile_definitions(DONT_ENABLE_DISABLED_RAMBANKS=1)
endif()

if(CONFIG_SOC_LPC55S69_CPU0)
  zephyr_sources_ifdef(CONFIG_PM power.c)
endif()

zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")

# workaround:
# Only IAR defines the symbol __Vectors, there is not good way to detect
# whether the symbol is defined.
if(NOT CMAKE_C_COMPILER_ID MATCHES "IAR")
zephyr_linker_symbol(SYMBOL __Vectors EXPR "@_vector_table@")
endif()
