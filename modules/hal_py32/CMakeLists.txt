# Copyright (c) 2016 Linaro Limited
# Copyright (c) 2017 Erwin Rol <erwin@erwinrol.com>
# Copyright (c) 2020 Linaro Limited
# Copyright (c) 2020 STMicroelectronics
# Copyright (c) 2024 STMicroelectronics
#
# SPDX-License-Identifier: Apache-2.0

if(NOT CONFIG_SOC_SERIES_PY32F0X)
  return()
endif()

zephyr_library()

set(hal_dir ${ZEPHYR_HAL_PY32_MODULE_DIR}/py32/py32f0xx)
string(TOUPPER "${CONFIG_SOC}" py32_cpu)
string(REPLACE "X" "x" py32_cpu "${py32_cpu}")
zephyr_compile_definitions(
  ${py32_cpu}
  USE_HAL_DRIVER
  USE_FULL_LL_DRIVER
  FORBID_VECT_TAB_MIGRATION
)
zephyr_compile_definitions_ifdef(CONFIG_USE_PY32_ASSERT USE_FULL_ASSERT)
zephyr_include_directories(${hal_dir}/drivers/include)

zephyr_library_sources(${hal_dir}/soc/system_py32f0xx.c)
zephyr_library_sources(${hal_dir}/drivers/src/py32f0xx_hal.c)
zephyr_library_sources(${hal_dir}/drivers/src/py32f0xx_hal_rcc.c)
zephyr_library_sources(${hal_dir}/drivers/src/py32f0xx_hal_rcc_ex.c)
zephyr_library_sources_ifdef(CONFIG_USE_PY32_HAL_ADC ${hal_dir}/drivers/src/py32f0xx_hal_adc.c)
zephyr_library_sources_ifdef(CONFIG_USE_PY32_HAL_ADC_EX
  ${hal_dir}/drivers/src/py32f0xx_hal_adc_ex.c
)
zephyr_library_sources_ifdef(CONFIG_USE_PY32_HAL_COMP ${hal_dir}/drivers/src/py32f0xx_hal_comp.c)
zephyr_library_sources_ifdef(CONFIG_USE_PY32_HAL_CORTEX
  ${hal_dir}/drivers/src/py32f0xx_hal_cortex.c
)
zephyr_library_sources_ifdef(CONFIG_USE_PY32_HAL_CRC ${hal_dir}/drivers/src/py32f0xx_hal_crc.c)
zephyr_library_sources_ifdef(CONFIG_USE_PY32_HAL_DMA ${hal_dir}/drivers/src/py32f0xx_hal_dma.c)
zephyr_library_sources_ifdef(CONFIG_USE_PY32_HAL_EXTI ${hal_dir}/drivers/src/py32f0xx_hal_exti.c)
zephyr_library_sources_ifdef(CONFIG_USE_PY32_HAL_FLASH ${hal_dir}/drivers/src/py32f0xx_hal_flash.c)
zephyr_library_sources_ifdef(CONFIG_USE_PY32_HAL_GPIO ${hal_dir}/drivers/src/py32f0xx_hal_gpio.c)
zephyr_library_sources_ifdef(CONFIG_USE_PY32_HAL_I2C ${hal_dir}/drivers/src/py32f0xx_hal_i2c.c)
zephyr_library_sources_ifdef(CONFIG_USE_PY32_HAL_IRDA ${hal_dir}/drivers/src/py32f0xx_hal_irda.c)
zephyr_library_sources_ifdef(CONFIG_USE_PY32_HAL_IWDG ${hal_dir}/drivers/src/py32f0xx_hal_iwdg.c)
zephyr_library_sources_ifdef(CONFIG_USE_PY32_HAL_LPTIM ${hal_dir}/drivers/src/py32f0xx_hal_lptim.c)
zephyr_library_sources_ifdef(CONFIG_USE_PY32_HAL_PWR ${hal_dir}/drivers/src/py32f0xx_hal_pwr.c)
zephyr_library_sources_ifdef(CONFIG_USE_PY32_HAL_RTC ${hal_dir}/drivers/src/py32f0xx_hal_rtc.c)
zephyr_library_sources_ifdef(CONFIG_USE_PY32_HAL_RTC_EX
  ${hal_dir}/drivers/src/py32f0xx_hal_rtc_ex.c
)
zephyr_library_sources_ifdef(CONFIG_USE_PY32_HAL_SPI ${hal_dir}/drivers/src/py32f0xx_hal_spi.c)
zephyr_library_sources_ifdef(CONFIG_USE_PY32_HAL_TIM ${hal_dir}/drivers/src/py32f0xx_hal_tim.c)
zephyr_library_sources_ifdef(CONFIG_USE_PY32_HAL_TIM_EX
  ${hal_dir}/drivers/src/py32f0xx_hal_tim_ex.c
)
zephyr_library_sources_ifdef(CONFIG_USE_PY32_HAL_UART ${hal_dir}/drivers/src/py32f0xx_hal_uart.c)
zephyr_library_sources_ifdef(CONFIG_USE_PY32_HAL_USART ${hal_dir}/drivers/src/py32f0xx_hal_usart.c)
zephyr_library_sources_ifdef(CONFIG_USE_PY32_HAL_WWDG ${hal_dir}/drivers/src/py32f0xx_hal_wwdg.c)
zephyr_library_sources_ifdef(CONFIG_USE_PY32_LL_ADC ${hal_dir}/drivers/src/py32f0xx_ll_adc.c)
zephyr_library_sources_ifdef(CONFIG_USE_PY32_LL_COMP ${hal_dir}/drivers/src/py32f0xx_ll_comp.c)
zephyr_library_sources_ifdef(CONFIG_USE_PY32_LL_CRC ${hal_dir}/drivers/src/py32f0xx_ll_crc.c)
zephyr_library_sources_ifdef(CONFIG_USE_PY32_LL_DMA ${hal_dir}/drivers/src/py32f0xx_ll_dma.c)
zephyr_library_sources_ifdef(CONFIG_USE_PY32_LL_EXTI ${hal_dir}/drivers/src/py32f0xx_ll_exti.c)
zephyr_library_sources_ifdef(CONFIG_USE_PY32_LL_GPIO ${hal_dir}/drivers/src/py32f0xx_ll_gpio.c)
zephyr_library_sources_ifdef(CONFIG_USE_PY32_LL_I2C ${hal_dir}/drivers/src/py32f0xx_ll_i2c.c)
zephyr_library_sources_ifdef(CONFIG_USE_PY32_LL_LPTIM ${hal_dir}/drivers/src/py32f0xx_ll_lptim.c)
zephyr_library_sources_ifdef(CONFIG_USE_PY32_LL_PWR ${hal_dir}/drivers/src/py32f0xx_ll_pwr.c)
zephyr_library_sources_ifdef(CONFIG_USE_PY32_LL_RCC ${hal_dir}/drivers/src/py32f0xx_ll_rcc.c)
zephyr_library_sources_ifdef(CONFIG_USE_PY32_LL_RTC ${hal_dir}/drivers/src/py32f0xx_ll_rtc.c)
zephyr_library_sources_ifdef(CONFIG_USE_PY32_LL_SPI ${hal_dir}/drivers/src/py32f0xx_ll_spi.c)
zephyr_library_sources_ifdef(CONFIG_USE_PY32_LL_TIM ${hal_dir}/drivers/src/py32f0xx_ll_tim.c)
zephyr_library_sources_ifdef(CONFIG_USE_PY32_LL_USART ${hal_dir}/drivers/src/py32f0xx_ll_usart.c)
zephyr_library_sources_ifdef(CONFIG_USE_PY32_LL_UTILS ${hal_dir}/drivers/src/py32f0xx_ll_utils.c)
