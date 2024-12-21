# Copyright (c) 2023 Cypress Semiconductor Corporation (an Infineon company) or
# an affiliate of Cypress Semiconductor Corporation
#
# SPDX-License-Identifier: Apache-2.0

set(hal_dir         ${ZEPHYR_HAL_INFINEON_MODULE_DIR}/mtb-hal-cat1)
set(hal_cat1a_dir   ${hal_dir}/COMPONENT_CAT1A)
set(hal_cat1b_dir   ${hal_dir}/COMPONENT_CAT1B)

# PSOC 6 family defines
zephyr_compile_definitions_ifdef(CONFIG_SOC_FAMILY_INFINEON_CAT1A COMPONENT_CAT1A)
zephyr_compile_definitions_ifdef(CONFIG_SOC_FAMILY_INFINEON_CAT1B COMPONENT_CAT1B)
zephyr_compile_definitions_ifdef(CONFIG_SOC_FAMILY_INFINEON_CAT1 COMPONENT_CAT1)
zephyr_compile_definitions_ifdef(CONFIG_SOC_FAMILY_INFINEON_CAT1 CY_USING_HAL)

# Add mtb-hal-cat1
zephyr_include_directories(${hal_dir}/include)
zephyr_include_directories(${hal_dir}/include_pvt)
zephyr_include_directories_ifdef(CONFIG_SOC_FAMILY_INFINEON_CAT1A ${hal_cat1a_dir}/include)
zephyr_include_directories_ifdef(CONFIG_SOC_FAMILY_INFINEON_CAT1B ${hal_cat1b_dir}/include)

# Packages sources
zephyr_library_sources_ifdef(CONFIG_SOC_PACKAGE_PSOC6_01_104_M_CSP_BLE
  ${hal_cat1a_dir}/source/pin_packages/cyhal_psoc6_01_104_m_csp_ble.c)
zephyr_library_sources_ifdef(CONFIG_SOC_PACKAGE_PSOC6_01_104_M_CSP_BLE_USB
  ${hal_cat1a_dir}/source/pin_packages/cyhal_psoc6_01_104_m_csp_ble_usb.c)
zephyr_library_sources_ifdef(CONFIG_SOC_PACKAGE_PSOC6_01_116_BGA_BLE
  ${hal_cat1a_dir}/source/pin_packages/cyhal_psoc6_01_116_bga_ble.c)
zephyr_library_sources_ifdef(CONFIG_SOC_PACKAGE_PSOC6_01_116_BGA_USB
  ${hal_cat1a_dir}/source/pin_packages/cyhal_psoc6_01_116_bga_usb.c)
zephyr_library_sources_ifdef(CONFIG_SOC_PACKAGE_PSOC6_01_124_BGA
  ${hal_cat1a_dir}/source/pin_packages/cyhal_psoc6_01_124_bga.c)
zephyr_library_sources_ifdef(CONFIG_SOC_PACKAGE_PSOC6_01_124_BGA_SIP
  ${hal_cat1a_dir}/source/pin_packages/cyhal_psoc6_01_124_bga_sip.c)
zephyr_library_sources_ifdef(CONFIG_SOC_PACKAGE_PSOC6_01_68_QFN_BLE
  ${hal_cat1a_dir}/source/pin_packages/cyhal_psoc6_01_68_qfn_ble.c)
zephyr_library_sources_ifdef(CONFIG_SOC_PACKAGE_PSOC6_01_80_WLCSP
  ${hal_cat1a_dir}/source/pin_packages/cyhal_psoc6_01_80_wlcsp.c)
zephyr_library_sources_ifdef(CONFIG_SOC_PACKAGE_PSOC6_02_100_WLCSP
  ${hal_cat1a_dir}/source/pin_packages/cyhal_psoc6_02_100_wlcsp.c)
zephyr_library_sources_ifdef(CONFIG_SOC_PACKAGE_PSOC6_02_124_BGA
  ${hal_cat1a_dir}/source/pin_packages/cyhal_psoc6_02_124_bga.c)
zephyr_library_sources_ifdef(CONFIG_SOC_PACKAGE_PSOC6_02_128_TQFP
  ${hal_cat1a_dir}/source/pin_packages/cyhal_psoc6_02_128_tqfp.c)
zephyr_library_sources_ifdef(CONFIG_SOC_PACKAGE_PSOC6_02_68_QFN
  ${hal_cat1a_dir}/source/pin_packages/cyhal_psoc6_02_68_qfn.c)
zephyr_library_sources_ifdef(CONFIG_SOC_PACKAGE_PSOC6_03_100_TQFP
  ${hal_cat1a_dir}/source/pin_packages/cyhal_psoc6_03_100_tqfp.c)
zephyr_library_sources_ifdef(CONFIG_SOC_PACKAGE_PSOC6_03_49_WLCSP
  ${hal_cat1a_dir}/source/pin_packages/cyhal_psoc6_03_49_wlcsp.c)
zephyr_library_sources_ifdef(CONFIG_SOC_PACKAGE_PSOC6_03_68_QFN
  ${hal_cat1a_dir}/source/pin_packages/cyhal_psoc6_03_68_qfn.c)
zephyr_library_sources_ifdef(CONFIG_SOC_PACKAGE_PSOC6_04_64_TQFP
  ${hal_cat1a_dir}/source/pin_packages/cyhal_psoc6_04_64_tqfp.c)
zephyr_library_sources_ifdef(CONFIG_SOC_PACKAGE_PSOC6_04_68_QFN
  ${hal_cat1a_dir}/source/pin_packages/cyhal_psoc6_04_68_qfn.c)
zephyr_library_sources_ifdef(CONFIG_SOC_PACKAGE_PSOC6_04_80_TQFP
  ${hal_cat1a_dir}/source/pin_packages/cyhal_psoc6_04_80_tqfp.c)

zephyr_library_sources_ifdef(CONFIG_SOC_PACKAGE_CYW20829_56_QFN
  ${hal_cat1b_dir}/source/pin_packages/cyhal_cyw20829_56_qfn.c)

zephyr_library_sources_ifdef(CONFIG_SOC_DIE_CYW20829
  ${hal_cat1b_dir}/source/triggers/cyhal_triggers_cyw20829.c)

zephyr_library_sources_ifdef(CONFIG_SOC_DIE_PSOC6_01
  ${hal_cat1a_dir}/source/triggers/cyhal_triggers_psoc6_01.c)
zephyr_library_sources_ifdef(CONFIG_SOC_DIE_PSOC6_02
  ${hal_cat1a_dir}/source/triggers/cyhal_triggers_psoc6_02.c)
zephyr_library_sources_ifdef(CONFIG_SOC_DIE_PSOC6_03
  ${hal_cat1a_dir}/source/triggers/cyhal_triggers_psoc6_03.c)
zephyr_library_sources_ifdef(CONFIG_SOC_DIE_PSOC6_04
  ${hal_cat1a_dir}/source/triggers/cyhal_triggers_psoc6_04.c)

# High level interface for interacting with CAT1 hardware
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_ADC     ${hal_dir}/source/cyhal_adc_sar.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_ADC     ${hal_dir}/source/cyhal_analog_common.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_FLASH   ${hal_dir}/source/cyhal_nvm.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_I2C     ${hal_dir}/source/cyhal_i2c.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_LPTIMER ${hal_dir}/source/cyhal_lptimer.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_RTC     ${hal_dir}/source/cyhal_rtc.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_SDIO    ${hal_dir}/source/cyhal_sdhc.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_SPI     ${hal_dir}/source/cyhal_spi.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_TIMER   ${hal_dir}/source/cyhal_timer.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_TRNG    ${hal_dir}/source/cyhal_trng.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_UART    ${hal_dir}/source/cyhal_uart.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_WDT     ${hal_dir}/source/cyhal_wdt.c)

if(CONFIG_USE_INFINEON_ADC OR CONFIG_USE_INFINEON_SMIF)
  zephyr_library_sources(${hal_dir}/source/cyhal_dma.c)
  zephyr_library_sources(${hal_dir}/source/cyhal_dma_dmac.c)
  zephyr_library_sources(${hal_dir}/source/cyhal_dma_dw.c)
endif()

if(CONFIG_USE_INFINEON_TIMER)
  zephyr_library_sources(${hal_dir}/source/cyhal_tcpwm_common.c)
endif()

if(CONFIG_USE_INFINEON_UART OR CONFIG_USE_INFINEON_I2C OR CONFIG_USE_INFINEON_SPI)
  zephyr_library_sources(${hal_dir}/source/cyhal_scb_common.c)
endif()

# Common part
zephyr_library_sources(${hal_dir}/source/cyhal_clock.c)
zephyr_library_sources(${hal_dir}/source/cyhal_gpio.c)
zephyr_library_sources(${hal_dir}/source/cyhal_hwmgr.c)
zephyr_library_sources(${hal_dir}/source/cyhal_interconnect.c)
zephyr_library_sources(${hal_dir}/source/cyhal_syspm.c)
zephyr_library_sources(${hal_dir}/source/cyhal_system.c)
zephyr_library_sources(${hal_dir}/source/cyhal_utils.c)
zephyr_library_sources(${hal_dir}/source/cyhal_utils_impl.c)
zephyr_library_sources(${hal_dir}/source/cyhal_wdt.c)
zephyr_library_sources(${hal_dir}/source/cyhal_irq_impl.c)
