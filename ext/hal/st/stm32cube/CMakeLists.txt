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

  zephyr_sources(stm32f0xx/drivers/src/stm32f0xx_hal.c)
  zephyr_sources(stm32f0xx/drivers/src/stm32f0xx_hal_rcc.c)
  zephyr_sources_ifdef(CONFIG_CLOCK_CONTROL_STM32_CUBE stm32f0xx/drivers/src/stm32f0xx_ll_utils.c)
  zephyr_sources_ifdef(CONFIG_I2C_STM32_V2 stm32f0xx/drivers/src/stm32f0xx_ll_i2c.c)
  zephyr_sources_ifdef(CONFIG_I2C_STM32_V2 stm32f0xx/drivers/src/stm32f0xx_ll_rcc.c)
  zephyr_sources(stm32f0xx/soc/system_stm32f0xx.c)
endif()

if(CONFIG_SOC_SERIES_STM32F1X)
  zephyr_include_directories(
	stm32f1xx/soc
	stm32f1xx/drivers/include
	stm32f1xx/drivers/include/Legacy
	)

  zephyr_sources(stm32f1xx/drivers/src/stm32f1xx_hal.c)
  zephyr_sources(stm32f1xx/drivers/src/stm32f1xx_hal_rcc.c)
  zephyr_sources_ifdef(CONFIG_CLOCK_CONTROL_STM32_CUBE stm32f1xx/drivers/src/stm32f1xx_ll_utils.c)
  zephyr_sources_ifdef(CONFIG_PWM stm32f1xx/drivers/src/stm32f1xx_hal_tim.c)
  zephyr_sources_ifdef(CONFIG_I2C stm32f1xx/drivers/src/stm32f1xx_ll_i2c.c)
  zephyr_sources(stm32f1xx/soc/system_stm32f1xx.c)
endif()

if(CONFIG_SOC_SERIES_STM32F3X)
  zephyr_include_directories(
	stm32f3xx/soc
	stm32f3xx/drivers/include
	stm32f3xx/drivers/include/Legacy
	)

  zephyr_sources(stm32f3xx/drivers/src/stm32f3xx_hal.c)
  zephyr_sources(stm32f3xx/drivers/src/stm32f3xx_hal_rcc.c)
  zephyr_sources_ifdef(CONFIG_CLOCK_CONTROL_STM32_CUBE stm32f3xx/drivers/src/stm32f3xx_ll_utils.c)
  zephyr_sources_ifdef(CONFIG_SPI_STM32 stm32f3xx/drivers/src/stm32f3xx_ll_spi.c)
  zephyr_sources_ifdef(CONFIG_I2C stm32f3xx/drivers/src/stm32f3xx_ll_i2c.c)
  zephyr_sources_ifdef(CONFIG_I2C stm32f3xx/drivers/src/stm32f3xx_ll_rcc.c)
  zephyr_sources(stm32f3xx/soc/system_stm32f3xx.c)
endif()



if(CONFIG_SOC_SERIES_STM32F4X)
  zephyr_include_directories(
	stm32f4xx/soc
	stm32f4xx/drivers/include
	stm32f4xx/drivers/include/Legacy
	)

  zephyr_sources(stm32f4xx/drivers/src/stm32f4xx_hal.c)
  zephyr_sources(stm32f4xx/drivers/src/stm32f4xx_hal_rcc.c)
  zephyr_sources_ifdef(CONFIG_CLOCK_CONTROL_STM32_CUBE stm32f4xx/drivers/src/stm32f4xx_ll_utils.c)
  zephyr_sources_ifdef(CONFIG_PWM stm32f4xx/drivers/src/stm32f4xx_hal_tim.c)
  zephyr_sources_ifdef(CONFIG_I2C stm32f4xx/drivers/src/stm32f4xx_ll_i2c.c)
  zephyr_sources_ifdef(CONFIG_SPI_STM32 stm32f4xx/drivers/src/stm32f4xx_ll_spi.c)
  zephyr_sources_ifdef(CONFIG_ENTROPY_STM32_RNG stm32f4xx/drivers/src/stm32f4xx_ll_rng.c)
  zephyr_sources_ifdef(CONFIG_ETH_STM32_HAL stm32f4xx/drivers/src/stm32f4xx_hal_eth.c)
  zephyr_sources_ifdef(CONFIG_USB_DC_STM32
    stm32f4xx/drivers/src/stm32f4xx_ll_usb.c
    stm32f4xx/drivers/src/stm32f4xx_hal_pcd.c
    stm32f4xx/drivers/src/stm32f4xx_hal_pcd_ex.c
    )

  zephyr_sources(stm32f4xx/soc/system_stm32f4xx.c)
endif()

if(CONFIG_SOC_SERIES_STM32F7X)
  zephyr_include_directories(
	stm32f7xx/soc
	stm32f7xx/drivers/include
	stm32f7xx/drivers/include/Legacy
	)

  zephyr_sources(stm32f7xx/drivers/src/stm32f7xx_hal.c)
  zephyr_sources(stm32f7xx/soc/system_stm32f7xx.c)
endif()

if(CONFIG_SOC_SERIES_STM32L4X)
  zephyr_include_directories(
	stm32l4xx/soc
	stm32l4xx/drivers/include
	stm32l4xx/drivers/include/Legacy
	)
  zephyr_sources(stm32l4xx/drivers/src/stm32l4xx_hal.c)
  zephyr_sources(stm32l4xx/drivers/src/stm32l4xx_hal_rcc.c)
  zephyr_sources_ifdef(CONFIG_CLOCK_CONTROL_STM32_CUBE stm32l4xx/drivers/src/stm32l4xx_ll_utils.c)
  zephyr_sources_ifdef(CONFIG_PWM stm32l4xx/drivers/src/stm32l4xx_hal_tim.c)
  zephyr_sources_ifdef(CONFIG_I2C stm32l4xx/drivers/src/stm32l4xx_ll_i2c.c)
  zephyr_sources_ifdef(CONFIG_SPI_STM32 stm32l4xx/drivers/src/stm32l4xx_ll_spi.c)
  zephyr_sources(stm32l4xx/soc/system_stm32l4xx.c)
endif()

