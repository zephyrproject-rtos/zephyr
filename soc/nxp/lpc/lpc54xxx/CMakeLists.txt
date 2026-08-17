#
# Copyright (c) 2017, 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources(soc.c)
if(CONFIG_SOC_RESET_HOOK AND CONFIG_SOC_LPC54114)
  zephyr_library_sources(gcc/startup_LPC54114_cm4.S)
endif()

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/${ARCH}/include
  )


# CMSIS SystemInit allows us to skip enabling clock to SRAM2 bank via
# this compiler definition
if(NOT DEFINED CONFIG_LPC54XXX_SRAM2_CLOCK)
zephyr_compile_definitions(DONT_ENABLE_DISABLED_RAMBANKS=1)
endif()

zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")
