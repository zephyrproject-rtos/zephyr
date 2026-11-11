#
# Copyright (c) 2017, 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources_ifdef(CONFIG_CPU_HAS_CUSTOM_FIXED_SOC_MPU_REGIONS mpu_regions.c)

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/${ARCH}/include
)

if(CONFIG_SOC_LPC54114)
  add_subdirectory(lpc5411x)
elseif(CONFIG_SOC_LPC54628)
  add_subdirectory(lpc546xx)
endif()

# CMSIS SystemInit allows us to skip enabling the clock to the RAM banks that
# are disabled out of reset, via this compiler definition
if(NOT CONFIG_SOC_SERIES_LPC54XXX_SRAM_CLOCKS)
  zephyr_compile_definitions(DONT_ENABLE_DISABLED_RAMBANKS=1)
endif()

zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")
