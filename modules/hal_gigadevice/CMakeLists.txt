# Copyright (c) 2021 ATL-Electronics
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_HAS_GD32_HAL)

string(TOUPPER ${CONFIG_SOC} gd32_soc_uc)

set(gd32_soc_dir     ${ZEPHYR_HAL_GIGADEVICE_MODULE_DIR}/${CONFIG_SOC_SERIES})
if(CONFIG_SOC_FAMILY_GD32_ARM)
  set(gd32_soc_sys_dir   ${gd32_soc_dir}/cmsis/gd/${CONFIG_SOC_SERIES})
elseif(CONFIG_SOC_SERIES_GD32VF103)
  set(gd32_soc_sys_dir   ${gd32_soc_dir}/riscv)
  zephyr_include_directories(${gd32_soc_dir}/riscv/drivers)
endif()

set(gd32_std_dir     ${gd32_soc_dir}/standard_peripheral)
set(gd32_std_src_dir ${gd32_std_dir}/source)

zephyr_library_named(hal_gigadevice)

zephyr_compile_definitions(${gd32_soc_uc})

# The header files of GigaDevice firmware are reference HXTAL_VALUE.
# The HXTAL_VALUE has the possibility of being referenced from any files
# via that header files.
# Therefore, we need to define HXTAL_VALUE for all compilations.
if(${CONFIG_GD32_HXTAL_8MHZ})
  zephyr_compile_definitions(HXTAL_VALUE=8000000 HXTAL_VALUE_25M=8000000)
elseif(${CONFIG_GD32_HXTAL_25MHZ})
  zephyr_compile_definitions(HXTAL_VALUE=25000000 HXTAL_VALUE_8M=25000000)
endif()

if(CONFIG_GD32_HAS_IRC_32K)
  zephyr_compile_definitions(IRC32K_VALUE=${CONFIG_GD32_LOW_SPEED_IRC_FREQUENCY})
endif()

if(CONFIG_GD32_HAS_IRC_40K)
  zephyr_compile_definitions(IRC40K_VALUE=${CONFIG_GD32_LOW_SPEED_IRC_FREQUENCY})
endif()

# GD32E50X series HAL public headers require extra definitions
if(${CONFIG_SOC_SERIES_GD32E50X})
  zephyr_compile_definitions(GD32E50X)
  if (${CONFIG_SOC_GD32E507})
    zephyr_compile_definitions(GD32E50X_CL)
  endif()
endif()

# GD32A50X series HAL public headers require extra definitions
if(${CONFIG_SOC_SERIES_GD32A50X})
  zephyr_compile_definitions(GD32A50X)
endif()

# Global includes to be used outside hal_gigadevice
zephyr_include_directories(${gd32_soc_sys_dir}/include)
zephyr_include_directories(${gd32_std_dir}/include)
zephyr_include_directories(${ZEPHYR_HAL_GIGADEVICE_MODULE_DIR}/include)
zephyr_include_directories(${ZEPHYR_HAL_GIGADEVICE_MODULE_DIR}/common_include)

zephyr_library_sources(${gd32_soc_sys_dir}/source/system_${CONFIG_SOC_SERIES}.c)

zephyr_library_sources_ifdef(CONFIG_USE_GD32_ADC      ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_adc.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_BKP      ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_bkp.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_CAN      ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_can.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_CMP      ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_cmp.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_CEC      ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_cec.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_CRC      ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_crc.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_CTC      ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_ctc.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_DAC      ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_dac.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_DBG      ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_dbg.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_DCI      ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_dci.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_DMA      ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_dma.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_ENET     ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_enet.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_EXMC     ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_exmc.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_EXTI     ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_exti.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_FMC      ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_fmc.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_FWDGT    ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_fwdgt.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_GPIO     ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_gpio.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_I2C      ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_i2c.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_IPA      ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_ipa.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_IREF     ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_iref.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_MISC     ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_misc.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_PMU      ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_pmu.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_RCU      ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_rcu.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_RTC      ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_rtc.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_SDIO     ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_sdio.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_SHRTIMER ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_shrtimer.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_SPI      ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_spi.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_SYSCFG   ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_syscfg.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_TIMER    ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_timer.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_TLI      ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_tli.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_TMU      ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_tmu.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_TRNG     ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_trng.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_TSI      ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_tsi.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_USART    ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_usart.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_WWDGT    ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_wwdgt.c)

endif()
