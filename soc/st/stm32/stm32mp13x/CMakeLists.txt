# Copyright (c) 2025 STMicroelectronics
#
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(${ZEPHYR_BASE}/drivers)
zephyr_sources(
  soc.c
  )

zephyr_sources_ifdef(CONFIG_SOC_SERIES_STM32MP13X_DDR_INIT ddr.c)

if(CONFIG_SOC_SERIES_STM32MP13X_DDR_INIT)
  dt_comp_path(stm32mp13_ddr_path COMPATIBLE "st,stm32mp13-ddr")
  dt_prop(stm32mp13_ddr_size PATH "${stm32mp13_ddr_path}" PROPERTY "st,mem-size")

  if(stm32mp13_ddr_size EQUAL 0x20000000)
    set(stm32mp13_ddr_type DDR_TYPE_DDR3_4Gb)
  elseif(stm32mp13_ddr_size EQUAL 0x40000000)
    set(stm32mp13_ddr_type DDR_TYPE_DDR3_8Gb)
  endif()

  # zephyr_library() names the hal_stm32 library after its path relative to ZEPHYR_BASE.
  file(RELATIVE_PATH stm32cube_target ${ZEPHYR_BASE} ${ZEPHYR_HAL_STM32_MODULE_DIR}/stm32cube)
  string(REGEX REPLACE "[/:]" "__" stm32cube_target ${stm32cube_target})
  string(REGEX REPLACE "~" "-" stm32cube_target ${stm32cube_target})
  zephyr_compile_definitions(
    $<$<STREQUAL:$<TARGET_PROPERTY:NAME>,${stm32cube_target}>:${stm32mp13_ddr_type}>
  )
endif()

add_subdirectory(utils)

zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_a_r/scripts/linker.ld CACHE INTERNAL "")
