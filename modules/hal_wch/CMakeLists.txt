# Copyright (c) 2022 TOKITA Hiroshi <tokita.hiroshi@gmail.com>
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_HAS_CH32_HAL)

if(${CONFIG_SOC} STREQUAL "ch32v307")
  set(ch32_series "ch32v30x")
endif()

set(ch32_soc_dir     ${ZEPHYR_HAL_WCH_MODULE_DIR}/${ch32_series})

zephyr_library_named(hal_wch)

set(ch32_periph_src_dir ${ch32_soc_dir}/Peripheral/src)
zephyr_include_directories(${ch32_soc_dir}/Peripheral/inc)
zephyr_include_directories(${ch32_soc_dir}/Core)
zephyr_include_directories(${ch32_soc_dir}/Debug)
zephyr_include_directories(${ch32_soc_dir}/User)
zephyr_include_directories(${ZEPHYR_HAL_WCH_MODULE_DIR}/include)
zephyr_include_directories(${ZEPHYR_HAL_WCH_MODULE_DIR}/common_include)
if(${CONFIG_USE_GD32_COMPAT})
  zephyr_include_directories(${ZEPHYR_HAL_WCH_MODULE_DIR}/gd32_compat)
  set(gd32_compat_dir     ${ZEPHYR_HAL_WCH_MODULE_DIR}/gd32_compat)
endif()

zephyr_library_sources(${ch32_soc_dir}/User/system_${ch32_series}.c)

zephyr_library_sources_ifdef(CONFIG_USE_CH32_ADC      ${ch32_periph_src_dir}/${ch32_series}_adc.c)
zephyr_library_sources_ifdef(CONFIG_USE_CH32_BKP      ${ch32_periph_src_dir}/${ch32_series}_bkp.c)
zephyr_library_sources_ifdef(CONFIG_USE_CH32_CAN      ${ch32_periph_src_dir}/${ch32_series}_can.c)
zephyr_library_sources_ifdef(CONFIG_USE_CH32_CRC      ${ch32_periph_src_dir}/${ch32_series}_crc.c)
zephyr_library_sources_ifdef(CONFIG_USE_CH32_DAC      ${ch32_periph_src_dir}/${ch32_series}_dac.c)
zephyr_library_sources_ifdef(CONFIG_USE_CH32_DBGMCU   ${ch32_periph_src_dir}/${ch32_series}_dbgmcu.c)
zephyr_library_sources_ifdef(CONFIG_USE_CH32_DMA      ${ch32_periph_src_dir}/${ch32_series}_dma.c)
zephyr_library_sources_ifdef(CONFIG_USE_CH32_DVP      ${ch32_periph_src_dir}/${ch32_series}_dvp.c)
zephyr_library_sources_ifdef(CONFIG_USE_CH32_ETH      ${ch32_periph_src_dir}/${ch32_series}_eth.c)
zephyr_library_sources_ifdef(CONFIG_USE_CH32_EXTI     ${ch32_periph_src_dir}/${ch32_series}_exti.c)
zephyr_library_sources_ifdef(CONFIG_USE_CH32_FLASH    ${ch32_periph_src_dir}/${ch32_series}_flash.c)
zephyr_library_sources_ifdef(CONFIG_USE_CH32_FSMC     ${ch32_periph_src_dir}/${ch32_series}_fsmc.c)
zephyr_library_sources_ifdef(CONFIG_USE_CH32_GPIO     ${ch32_periph_src_dir}/${ch32_series}_gpio.c)
zephyr_library_sources_ifdef(CONFIG_USE_CH32_I2C      ${ch32_periph_src_dir}/${ch32_series}_i2c.c)
zephyr_library_sources_ifdef(CONFIG_USE_CH32_IWDG     ${ch32_periph_src_dir}/${ch32_series}_iwdg.c)
zephyr_library_sources_ifdef(CONFIG_USE_CH32_MISC     ${ch32_periph_src_dir}/${ch32_series}_misc.c)
zephyr_library_sources_ifdef(CONFIG_USE_CH32_OPA      ${ch32_periph_src_dir}/${ch32_series}_opa.c)
zephyr_library_sources_ifdef(CONFIG_USE_CH32_PWR      ${ch32_periph_src_dir}/${ch32_series}_pwr.c)
zephyr_library_sources_ifdef(CONFIG_USE_CH32_RCC      ${ch32_periph_src_dir}/${ch32_series}_rcc.c)
zephyr_library_sources_ifdef(CONFIG_USE_CH32_RNG      ${ch32_periph_src_dir}/${ch32_series}_rng.c)
zephyr_library_sources_ifdef(CONFIG_USE_CH32_RTC      ${ch32_periph_src_dir}/${ch32_series}_rtc.c)
zephyr_library_sources_ifdef(CONFIG_USE_CH32_SDIO     ${ch32_periph_src_dir}/${ch32_series}_sdio.c)
zephyr_library_sources_ifdef(CONFIG_USE_CH32_SPI      ${ch32_periph_src_dir}/${ch32_series}_spi.c)
zephyr_library_sources_ifdef(CONFIG_USE_CH32_TIM      ${ch32_periph_src_dir}/${ch32_series}_tim.c)
zephyr_library_sources_ifdef(CONFIG_USE_CH32_USART    ${ch32_periph_src_dir}/${ch32_series}_usart.c)
zephyr_library_sources_ifdef(CONFIG_USE_CH32_WWDG     ${ch32_periph_src_dir}/${ch32_series}_wwdg.c)

zephyr_library_sources_ifdef(CONFIG_USE_GD32_USART    ${gd32_compat_dir}/gd32_usart.c)


endif()
