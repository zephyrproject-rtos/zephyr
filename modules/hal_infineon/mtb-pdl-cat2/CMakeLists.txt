# Copyright (c) 2025 Infineon Technologies AG,
# or an affiliate of Infineon Technologies AG.
#
# SPDX-License-Identifier: Apache-2.0

# Infineon PSoC4100tp / CAT2 MCUs

# PDL directories
set(pdl_dir           ${ZEPHYR_HAL_INFINEON_MODULE_DIR}/mtb-pdl-cat2)
set(pdl_drv_dir       ${pdl_dir}/drivers)
set(pdl_dev_cat2_dir  ${pdl_dir}/devices)

# Generate PDL specific SOC defines
zephyr_compile_definitions(${CONFIG_SOC_PART_NUMBER})
zephyr_compile_definitions_ifdef(CONFIG_SOC_FAMILY_INFINEON_CAT2 $<UPPER_CASE:${CONFIG_SOC}>)

# Generate PDL specific define (w. *_device) for SOC module
zephyr_compile_definitions_ifdef(CONFIG_SOC_PART_NUMBER ${CONFIG_SOC_PART_NUMBER}_device)

# Include directories
zephyr_include_directories(${pdl_drv_dir}/include)
zephyr_include_directories_ifdef(CONFIG_SOC_FAMILY_INFINEON_CAT2 ${pdl_dev_cat2_dir}/include)
zephyr_include_directories_ifdef(CONFIG_SOC_FAMILY_INFINEON_CAT2 ${pdl_dev_cat2_dir}/include/ip)
zephyr_include_directories(${ZEPHYR_HAL_INFINEON_MODULE_DIR}/core-lib/include)

# Tool Chain
zephyr_library_sources(${pdl_drv_dir}/source/COMPONENT_CM0P/TOOLCHAIN_GCC_ARM/cy_syslib_gcc.S)

# Peripheral drivers
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_ADC      ${pdl_drv_dir}/source/cy_sar.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_I2C      ${pdl_drv_dir}/source/cy_scb_i2c.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_RTC      ${pdl_drv_dir}/source/cy_rtc.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_LP_COMP  ${pdl_drv_dir}/source/cy_lpcomp.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_MSCLP    ${pdl_drv_dir}/source/cy_msclp.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_SPI      ${pdl_drv_dir}/source/cy_scb_spi.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_TIMER    ${pdl_drv_dir}/source/cy_tcpwm_counter.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_TCPWM_QUADDEC ${pdl_drv_dir}/cy_tcpwm_quaddec.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_UART     ${pdl_drv_dir}/source/cy_scb_uart.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_FLASH    ${pdl_drv_dir}/source/cy_flash.c)

# SmartIO
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_SMARTIO  ${pdl_drv_dir}/cy_smartio.c)

# TRNG / Crypto
if(CONFIG_USE_INFINEON_TRNG)
  zephyr_library_sources(${pdl_drv_dir}/source/cy_crypto_trng.c)
  zephyr_library_sources(${pdl_drv_dir}/source/cy_cryptolite_trng.c)
endif()

# SCB common sources (I2C/SPI/UART)
if(CONFIG_USE_INFINEON_UART OR CONFIG_USE_INFINEON_I2C OR CONFIG_USE_INFINEON_SPI)
  zephyr_library_sources(${pdl_drv_dir}/source/cy_scb_common.c)
endif()

# DMA
if(CONFIG_USE_INFINEON_DMA OR CONFIG_USE_INFINEON_ADC)
  zephyr_library_sources(${pdl_drv_dir}/source/cy_dmac.c)
endif()

# Watch Dog Timer
if(CONFIG_USE_INFINEON_WDT)
  zephyr_library_sources(${pdl_drv_dir}/source/cy_wdt.c)
  zephyr_library_sources(${pdl_drv_dir}/source/cy_crwdt.c)
endif()

# Common part
zephyr_library_sources(${pdl_drv_dir}/source/cy_gpio.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_sysclk.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_syspm.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_syslib.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_systick.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_trigmux.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_sysint.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_flash.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_wdt.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_tcpwm_pwm.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_ram.c)
