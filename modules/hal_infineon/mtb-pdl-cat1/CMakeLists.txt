# Copyright (c) 2022 Cypress Semiconductor Corporation.
#
# SPDX-License-Identifier: Apache-2.0

set(pdl_dir             ${ZEPHYR_HAL_INFINEON_MODULE_DIR}/mtb-pdl-cat1)
set(pdl_drv_dir         ${ZEPHYR_HAL_INFINEON_MODULE_DIR}/mtb-pdl-cat1/drivers)
set(pdl_dev_cat1a_dir   ${ZEPHYR_HAL_INFINEON_MODULE_DIR}/mtb-pdl-cat1/devices/COMPONENT_CAT1A)

zephyr_compile_definitions(${CONFIG_SOC_PART_NUMBER})

# Add mtb-pdl-cat1
zephyr_include_directories(${pdl_drv_dir}/include)
zephyr_include_directories(${pdl_dev_cat1a_dir}/include)
zephyr_include_directories(${pdl_dev_cat1a_dir}/include/ip)
zephyr_include_directories(${pdl_dev_cat1a_dir}/templates/COMPONENT_MTB)

zephyr_library_sources(${pdl_dev_cat1a_dir}/source/cy_device.c)
zephyr_library_sources(${pdl_drv_dir}/source/TOOLCHAIN_GCC_ARM/cy_syslib_gcc.S)

zephyr_library_sources_ifdef(CONFIG_CPU_CORTEX_M4
  ${pdl_dev_cat1a_dir}/templates/COMPONENT_MTB/COMPONENT_CM4/system_psoc6_cm4.c)
zephyr_library_sources_ifdef(CONFIG_CPU_CORTEX_M0PLUS
  ${pdl_dev_cat1a_dir}/templates/COMPONENT_MTB/COMPONENT_CM0P/system_psoc6_cm0plus.c)

# Peripheral drivers
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_UART     ${pdl_drv_dir}/source/cy_scb_uart.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_ADC      ${pdl_drv_dir}/source/cy_sar.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_I2C      ${pdl_drv_dir}/source/cy_scb_i2c.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_SPI      ${pdl_drv_dir}/source/cy_scb_spi.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_TIMER    ${pdl_drv_dir}/source/cy_tcpwm_counter.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_PWM      ${pdl_drv_dir}/source/cy_tcpwm_pwm.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_LPTIMER  ${pdl_drv_dir}/source/cy_mcwdt.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_RTC      ${pdl_drv_dir}/source/cy_rtc.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_SDIO     ${pdl_drv_dir}/source/cy_sd_host.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FAMILY_PSOC6      ${pdl_drv_dir}/source/cy_sysint.c)

if(CONFIG_USE_INFINEON_TRNG)
  zephyr_library_sources(${pdl_drv_dir}/source/cy_crypto.c)
  zephyr_library_sources(${pdl_drv_dir}/source/cy_crypto_core_trng_v1.c)
  zephyr_library_sources(${pdl_drv_dir}/source/cy_crypto_core_trng_v2.c)
endif()

if(CONFIG_USE_INFINEON_UART OR CONFIG_USE_INFINEON_I2C OR CONFIG_USE_INFINEON_SPI)
  zephyr_library_sources(${pdl_drv_dir}/source/cy_scb_common.c)
endif()

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
zephyr_library_sources(${pdl_drv_dir}/source/cy_syslib.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_syspm.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_systick.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_trigmux.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_wdt.c)
