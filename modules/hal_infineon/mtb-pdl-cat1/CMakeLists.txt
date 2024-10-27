# Copyright (c) 2023 Cypress Semiconductor Corporation (an Infineon company) or
# an affiliate of Cypress Semiconductor Corporation
#
# SPDX-License-Identifier: Apache-2.0

set(pdl_dir             ${ZEPHYR_HAL_INFINEON_MODULE_DIR}/mtb-pdl-cat1)
set(pdl_drv_dir         ${ZEPHYR_HAL_INFINEON_MODULE_DIR}/mtb-pdl-cat1/drivers)
set(pdl_dev_cat1a_dir   ${ZEPHYR_HAL_INFINEON_MODULE_DIR}/mtb-pdl-cat1/devices/COMPONENT_CAT1A)
set(pdl_dev_cat1b_dir   ${ZEPHYR_HAL_INFINEON_MODULE_DIR}/mtb-pdl-cat1/devices/COMPONENT_CAT1B)


# Generate PDL specific SOC defines
zephyr_compile_definitions(${CONFIG_SOC_PART_NUMBER})
zephyr_compile_definitions_ifdef(CONFIG_SOC_FAMILY_INFINEON_CAT1 $<UPPER_CASE:${CONFIG_SOC}>)

# Generate PDL specific define (w. *_device) for SOC module (e.g: CYBLE_416045_02)
zephyr_compile_definitions_ifdef(CONFIG_SOC_CYBLE_416045_02 CYBLE_416045_02_device)

# Add mtb-pdl-cat1
zephyr_include_directories(${pdl_drv_dir}/include)

zephyr_include_directories_ifdef(CONFIG_SOC_FAMILY_INFINEON_CAT1A ${pdl_dev_cat1a_dir}/include)
zephyr_include_directories_ifdef(CONFIG_SOC_FAMILY_INFINEON_CAT1A ${pdl_dev_cat1a_dir}/include/ip)
zephyr_library_sources_ifdef(CONFIG_SOC_FAMILY_INFINEON_CAT1A ${pdl_dev_cat1a_dir}/source/cy_device.c)


zephyr_include_directories(${pdl_drv_dir}/include)
zephyr_include_directories_ifdef(CONFIG_SOC_FAMILY_INFINEON_CAT1B ${pdl_dev_cat1b_dir}/include)
zephyr_include_directories_ifdef(CONFIG_SOC_FAMILY_INFINEON_CAT1B ${pdl_dev_cat1b_dir}/include/ip)
zephyr_library_sources_ifdef(CONFIG_SOC_FAMILY_INFINEON_CAT1B ${pdl_dev_cat1b_dir}/source/cy_device.c)

zephyr_library_sources(${pdl_drv_dir}/source/TOOLCHAIN_GCC_ARM/cy_syslib_ext.S)

# Peripheral drivers
zephyr_library_sources_ifdef(CONFIG_SOC_FAMILY_PSOC6_LEGACY      ${pdl_drv_dir}/source/cy_sysint.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_ADC      ${pdl_drv_dir}/source/cy_sar.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_I2C      ${pdl_drv_dir}/source/cy_scb_i2c.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_LPTIMER  ${pdl_drv_dir}/source/cy_mcwdt.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_PWM      ${pdl_drv_dir}/source/cy_tcpwm_pwm.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_RTC      ${pdl_drv_dir}/source/cy_rtc.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_SDIO     ${pdl_drv_dir}/source/cy_sd_host.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_SPI      ${pdl_drv_dir}/source/cy_scb_spi.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_TIMER    ${pdl_drv_dir}/source/cy_tcpwm_counter.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_UART     ${pdl_drv_dir}/source/cy_scb_uart.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_FLASH    ${pdl_drv_dir}/source/cy_flash.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_WDT      ${pdl_drv_dir}/source/cy_wdt.c)

if(CONFIG_SOC_FAMILY_INFINEON_CAT1B)
  zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_ADC      ${pdl_drv_dir}/source/cy_adcmic.c)
else()
  zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_ADC      ${pdl_drv_dir}/source/cy_sar.c)
endif()

if(CONFIG_USE_INFINEON_TRNG)
  zephyr_library_sources(${pdl_drv_dir}/source/cy_crypto.c)
  zephyr_library_sources(${pdl_drv_dir}/source/cy_crypto_core_trng_v1.c)
  zephyr_library_sources(${pdl_drv_dir}/source/cy_crypto_core_trng_v2.c)
endif()

if(CONFIG_USE_INFINEON_UART OR CONFIG_USE_INFINEON_I2C OR CONFIG_USE_INFINEON_SPI)
  zephyr_library_sources(${pdl_drv_dir}/source/cy_scb_common.c)
endif()

if(CONFIG_USE_INFINEON_ADC OR CONFIG_USE_INFINEON_SMIF)
  zephyr_library_sources(${pdl_drv_dir}/source/cy_dma.c)
  zephyr_library_sources(${pdl_drv_dir}/source/cy_dmac.c)
endif()

zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_SMIF ${pdl_drv_dir}/source/cy_smif.c)
zephyr_code_relocate(FILES ${pdl_drv_dir}/source/cy_smif.c LOCATION RAM)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_SMIF ${pdl_drv_dir}/source/cy_smif_sfdp.c)
zephyr_code_relocate(FILES ${pdl_drv_dir}/source/cy_smif_sfdp.c LOCATION RAM)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_SMIF ${pdl_drv_dir}/source/cy_smif_memslot.c)
zephyr_code_relocate(FILES ${pdl_drv_dir}/source/cy_smif_memslot.c LOCATION RAM)

if(CONFIG_USE_INFINEON_ADC)
  zephyr_library_sources(${pdl_drv_dir}/source/cy_sysanalog.c)
endif()

# Common part
zephyr_library_sources(${pdl_drv_dir}/source/cy_flash.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_gpio.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_ipc_drv.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_ipc_pipe.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_ipc_sema.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_prot.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_sysclk.c)
if(CONFIG_SOC_FAMILY_INFINEON_CAT1B)
  zephyr_library_sources(${pdl_drv_dir}/source/cy_sysclk_v2.c)
  zephyr_library_sources(${pdl_drv_dir}/source/cy_systick_v2.c)
  zephyr_library_sources(${pdl_drv_dir}/source/cy_syspm_v2.c)
  zephyr_library_sources(${pdl_drv_dir}/source/cy_syspm_btss.c)
endif()
zephyr_library_sources(${pdl_drv_dir}/source/cy_syslib.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_syspm.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_systick.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_trigmux.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_wdt.c)

# add IPC_BT driver for CYW208XX devices
zephyr_library_sources_ifdef(CONFIG_BT_CYW208XX  ${pdl_drv_dir}/source/cy_ipc_bt.c)
zephyr_library_sources_ifdef(CONFIG_BT_CYW208XX  ${pdl_drv_dir}/source/cy_syspm_pdcm.c)
