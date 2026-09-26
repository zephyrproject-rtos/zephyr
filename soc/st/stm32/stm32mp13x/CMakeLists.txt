# Copyright (c) 2025 STMicroelectronics
#
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(${ZEPHYR_BASE}/drivers)
zephyr_sources(
  soc.c
  )

zephyr_sources_ifdef(CONFIG_SOC_SERIES_STM32MP13X_DDR_INIT ddr.c)

if(CONFIG_SOC_SERIES_STM32MP13X_DDR_INIT)
  set(stm32cube_target "..__modules__hal__stm32__stm32cube")
  set(stm32mp13_ddr_config ${CMAKE_CURRENT_SOURCE_DIR}/stm32mp13_ddr_config.h)
  set(stm32cube_c_source
    "$<AND:$<COMPILE_LANGUAGE:C>,$<STREQUAL:$<TARGET_PROPERTY:NAME>,${stm32cube_target}>>"
    )

  zephyr_compile_options(
    $<${stm32cube_c_source}:-include>
    $<${stm32cube_c_source}:${stm32mp13_ddr_config}>
    )
endif()

add_subdirectory(utils)

zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_a_r/scripts/linker.ld CACHE INTERNAL "")
