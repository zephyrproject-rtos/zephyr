# Copyright (c) 2021 ATL-Electronics
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_HAS_GD32_HAL)

zephyr_library_named(hal_gigadevice)

zephyr_library_compile_definitions(${gd32_soc_uc})

set(gd32_soc_dir     ${ZEPHYR_HAL_GIGADEVICE_MODULE_DIR}/${gd32_soc_uc})
set(gd32_cmsis_dir   ${gd32_soc_dir}/CMSIS/GD/${gd32_soc_uc})
set(gd32_std_dir     ${gd32_soc_dir}/standard_peripheral)
set(gd32_std_src_dir ${gd32_std_dir}/Source)

# Global includes to be used outside hal_gigadevice
zephyr_include_directories(${gd32_cmsis_dir}/Include)
zephyr_include_directories(${gd32_std_dir}/Include)

zephyr_library_sources(${gd32_cmsis_dir}/Source/system_${gd32_soc_lc}.c)

zephyr_library_sources_ifdef(CONFIG_USE_GD32_ADC   ${gd32_std_src_dir}/${gd32_soc_lc}_adc.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_BKP   ${gd32_std_src_dir}/${gd32_soc_lc}_bkp.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_CAN   ${gd32_std_src_dir}/${gd32_soc_lc}_can.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_CRC   ${gd32_std_src_dir}/${gd32_soc_lc}_crc.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_CTC   ${gd32_std_src_dir}/${gd32_soc_lc}_ctc.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_DAC   ${gd32_std_src_dir}/${gd32_soc_lc}_dac.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_DBG   ${gd32_std_src_dir}/${gd32_soc_lc}_dbg.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_DMA   ${gd32_std_src_dir}/${gd32_soc_lc}_dma.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_EXMC  ${gd32_std_src_dir}/${gd32_soc_lc}_exmc.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_EXTI  ${gd32_std_src_dir}/${gd32_soc_lc}_exti.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_FMC   ${gd32_std_src_dir}/${gd32_soc_lc}_fmc.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_FWDGT ${gd32_std_src_dir}/${gd32_soc_lc}_fwdgt.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_GPIO  ${gd32_std_src_dir}/${gd32_soc_lc}_gpio.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_I2C   ${gd32_std_src_dir}/${gd32_soc_lc}_i2c.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_MISC  ${gd32_std_src_dir}/${gd32_soc_lc}_misc.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_PMU   ${gd32_std_src_dir}/${gd32_soc_lc}_pmu.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_RCU   ${gd32_std_src_dir}/${gd32_soc_lc}_rcu.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_RTC   ${gd32_std_src_dir}/${gd32_soc_lc}_rtc.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_SDIO  ${gd32_std_src_dir}/${gd32_soc_lc}_sdio.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_SPI   ${gd32_std_src_dir}/${gd32_soc_lc}_spi.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_TIMER ${gd32_std_src_dir}/${gd32_soc_lc}_timer.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_USART ${gd32_std_src_dir}/${gd32_soc_lc}_usart.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_WWDGT ${gd32_std_src_dir}/${gd32_soc_lc}_wwdgt.c)

endif()
