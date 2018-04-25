# Makefile - STM32Cube SDK
#
# Copyright (c) 2016 Linaro Limited
#
# SPDX-License-Identifier: Apache-2.0

#add_library(STM32CUBE INTERFACE)
string(TOUPPER ${CONFIG_SOC} _STM32CUBE_CPU)
string(REPLACE "X" "x" STM32CUBE_CPU ${_STM32CUBE_CPU})

# STM32Cub uses the CPU name to expose SOC-specific attributes of a specific
# peripheral.
# It also requires USE_HAL_DRIVER to be define in order to benefit from
# STM32Cube HAL and LL APIs
zephyr_compile_definitions(
  -D${STM32CUBE_CPU}
  -DUSE_HAL_DRIVER
  -DUSE_FULL_LL_DRIVER
  )

set(stm_socs
  stm32f0x
  stm32f1x
  stm32f2x
  stm32f3x
  stm32f4x
  stm32f7x
  stm32l0x
  stm32l4x
  )

foreach(stm_soc ${stm_socs})
  string(TOUPPER ${stm_soc} soc_to_upper)
  if(CONFIG_SOC_SERIES_${soc_to_upper})
    zephyr_include_directories(
	  ${stm_soc}x/soc
	  ${stm_soc}x/drivers/include
	  ${stm_soc}x/drivers/include/Legacy
	  )

    add_subdirectory(${stm_soc}x)
  endif()
endforeach()
