#
# Copyright (c) 2017, 2024 NXP
# Copyright (c) 2024, VCI Development - LPC54S018 support
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources(soc.c)
zephyr_library_sources_ifdef(CONFIG_SOC_RESET_HOOK gcc/startup_LPC54S018_cm4.S)

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/${ARCH}/include
  )


# Critical CPU definitions for MCUX SDK integration (based on FreeRTOS analysis)
zephyr_compile_definitions(
  CPU_LPC54S018JET180=1
  CPU_LPC54S018JET180_cm4
  LPC54S018M_SERIES
  __USE_CMSIS
  MCUXPRESSO_SDK
)

# CMSIS SystemInit allows us to skip enabling clock to SRAM2 bank via
# this compiler definition
if(NOT DEFINED CONFIG_LPC54XXX_SRAM2_CLOCK)
zephyr_compile_definitions(DONT_ENABLE_DISABLED_RAMBANKS=1)
endif()

zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")