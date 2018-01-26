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

if(CONFIG_SOC_SERIES_STM32F0X)
  zephyr_include_directories(
	stm32f0xx/soc
	stm32f0xx/drivers/include
	stm32f0xx/drivers/include/Legacy
	)

  add_subdirectory(stm32f0xx)
endif()

if(CONFIG_SOC_SERIES_STM32F1X)
  zephyr_include_directories(
	stm32f1xx/soc
	stm32f1xx/drivers/include
	stm32f1xx/drivers/include/Legacy
	)

  add_subdirectory(stm32f1xx)
endif()

if(CONFIG_SOC_SERIES_STM32F3X)
  zephyr_include_directories(
	stm32f3xx/soc
	stm32f3xx/drivers/include
	stm32f3xx/drivers/include/Legacy
	)

  add_subdirectory(stm32f3xx)
endif()

if(CONFIG_SOC_SERIES_STM32F4X)
  zephyr_include_directories(
	stm32f4xx/soc
	stm32f4xx/drivers/include
	stm32f4xx/drivers/include/Legacy
	)

  add_subdirectory(stm32f4xx)
endif()

if(CONFIG_SOC_SERIES_STM32F7X)
  zephyr_include_directories(
	stm32f7xx/soc
	stm32f7xx/drivers/include
	stm32f7xx/drivers/include/Legacy
	)

  add_subdirectory(stm32f7xx)
endif()

if(CONFIG_SOC_SERIES_STM32L4X)
  zephyr_include_directories(
	stm32l4xx/soc
	stm32l4xx/drivers/include
	stm32l4xx/drivers/include/Legacy
	)

  add_subdirectory(stm32l4xx)
endif()

