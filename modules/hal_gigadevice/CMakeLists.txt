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

# Use enhanced DMA descriptors for ENET.
zephyr_compile_definitions(SELECT_DESCRIPTORS_ENHANCED_MODE=1)

# PHY configuration.
zephyr_compile_definitions(_PHY_H_=1)
# PHY address determined by the hardware
zephyr_compile_definitions(PHY_ADDRESS=${CONFIG_ETH_GD32_HAL_PHY_ADDRESS})
# PHY read write timeouts
zephyr_compile_definitions(PHY_READ_TO=0x0004FFFFU)
zephyr_compile_definitions(PHY_WRITE_TO=0x0004FFFFU)
# PHY delay
zephyr_compile_definitions(PHY_RESETDELAY=0x008FFFFFU)
zephyr_compile_definitions(PHY_CONFIGDELAY=0x00FFFFFFU)
# PHY register address
zephyr_compile_definitions(PHY_REG_BCR=0U)
zephyr_compile_definitions(PHY_REG_BSR=1U)
# PHY basic control register
zephyr_compile_definitions(PHY_RESET=0x8000U)
zephyr_compile_definitions(PHY_LOOPBACK=0x4000U)
zephyr_compile_definitions(PHY_FULLDUPLEX_100M=0x2100U)
zephyr_compile_definitions(PHY_HALFDUPLEX_100M=0x2000U)
zephyr_compile_definitions(PHY_FULLDUPLEX_10M=0x0100U)
zephyr_compile_definitions(PHY_HALFDUPLEX_10M=0x0000U)
zephyr_compile_definitions(PHY_AUTONEGOTIATION=0x1000U)
zephyr_compile_definitions(PHY_RESTART_AUTONEGOTIATION=0x0200U)
zephyr_compile_definitions(PHY_POWERDOWN=0x0800U)
zephyr_compile_definitions(PHY_ISOLATE=0x0400U)
# PHY basic status register
zephyr_compile_definitions(PHY_AUTONEGO_COMPLETE=0x0020U)
zephyr_compile_definitions(PHY_LINKED_STATUS=0x0004U)
zephyr_compile_definitions(PHY_JABBER_DETECTION=0x0002U)
# PHY transceiver status register
if(CONFIG_ETH_GD32_HAL_PHY_LAN8700)
  zephyr_compile_definitions(PHY_SR=31U)
  zephyr_compile_definitions(PHY_SPEED_STATUS=0x0004U)
  zephyr_compile_definitions(PHY_DUPLEX_STATUS=0x0010U)
elseif(CONFIG_ETH_GD32_HAL_PHY_DP83848)
  zephyr_compile_definitions(PHY_SR=16U)
  zephyr_compile_definitions(PHY_SPEED_STATUS=0x0002U)
  zephyr_compile_definitions(PHY_DUPLEX_STATUS=0x0004U)
endif()

# Global includes to be used outside hal_gigadevice
zephyr_include_directories(${gd32_soc_sys_dir}/include)
zephyr_include_directories(${gd32_std_dir}/include)
zephyr_include_directories(${ZEPHYR_HAL_GIGADEVICE_MODULE_DIR}/include)
zephyr_include_directories(${ZEPHYR_HAL_GIGADEVICE_MODULE_DIR}/common_include)

zephyr_library_sources(${gd32_soc_sys_dir}/source/system_${CONFIG_SOC_SERIES}.c)

zephyr_library_sources_ifdef(CONFIG_USE_GD32_ADC    ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_adc.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_BKP    ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_bkp.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_CAN    ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_can.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_CEC    ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_cec.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_CRC    ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_crc.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_CTC    ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_ctc.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_DAC    ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_dac.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_DBG    ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_dbg.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_DCI    ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_dci.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_DMA    ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_dma.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_ENET   ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_enet.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_EXMC   ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_exmc.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_EXTI   ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_exti.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_FMC    ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_fmc.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_FWDGT  ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_fwdgt.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_GPIO   ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_gpio.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_I2C    ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_i2c.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_IPA    ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_ipa.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_IREF   ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_iref.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_MISC   ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_misc.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_PMU    ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_pmu.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_RCU    ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_rcu.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_RTC    ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_rtc.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_SDIO   ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_sdio.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_SPI    ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_spi.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_SYSCFG ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_syscfg.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_TIMER  ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_timer.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_TLI    ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_tli.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_TRNG   ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_trng.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_TSI    ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_tsi.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_USART  ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_usart.c)
zephyr_library_sources_ifdef(CONFIG_USE_GD32_WWDGT  ${gd32_std_src_dir}/${CONFIG_SOC_SERIES}_wwdgt.c)

endif()
