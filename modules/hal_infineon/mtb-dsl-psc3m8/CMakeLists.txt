# SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
# SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

set(pdl_dir ${ZEPHYR_HAL_INFINEON_MODULE_DIR}/mtb-dsl-psc3m8/pdl)

set(pdl_drv_src_dir ${pdl_dir}/drivers/source)
set(pdl_dev_psc3m8_dir ${pdl_dir}/devices)

# Generate PDL specific SOC defines
zephyr_library_compile_definitions($<UPPER_CASE:${CONFIG_SOC}>)

# PDL sources here do not pull in infineon_kconfig.h, so repeat the SRF switch.
zephyr_library_compile_definitions(CY_SRF_DISABLE)

# Add mtb-pdl-psc3m8
zephyr_include_directories(
  ${pdl_dir}/drivers/include
  ${pdl_dev_psc3m8_dir}/include
  ${pdl_dev_psc3m8_dir}/include/ip
)
zephyr_library_sources(${pdl_dev_psc3m8_dir}/source/cy_device.c)

if(${ZEPHYR_TOOLCHAIN_VARIANT} STREQUAL "armclang")
  zephyr_library_sources(${pdl_drv_src_dir}/TOOLCHAIN_ARM/cy_syslib_ext.S)
else()
  zephyr_library_sources(${pdl_drv_src_dir}/TOOLCHAIN_GCC_ARM/cy_syslib_ext.S)
endif()

# Peripheral drivers
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_FLASH ${pdl_drv_src_dir}/cy_flash.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_I2C ${pdl_drv_src_dir}/cy_scb_i2c.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_LPTIMER ${pdl_drv_src_dir}/cy_mcwdt.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_PWM ${pdl_drv_src_dir}/cy_tcpwm_pwm.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_RTC ${pdl_drv_src_dir}/cy_rtc.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_SPI ${pdl_drv_src_dir}/cy_scb_spi.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_TIMER ${pdl_drv_src_dir}/cy_tcpwm_counter.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_UART ${pdl_drv_src_dir}/cy_scb_uart.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_WDT ${pdl_drv_src_dir}/cy_wdt.c)

if(CONFIG_USE_INFINEON_UART OR CONFIG_USE_INFINEON_I2C OR CONFIG_USE_INFINEON_SPI)
  zephyr_library_sources(${pdl_drv_src_dir}/cy_scb_common.c)
endif()

zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_DMA ${pdl_drv_src_dir}/cy_dma.c)

# Common part
zephyr_library_sources(
  ${pdl_drv_src_dir}/cy_syspm_v2.c
  ${pdl_drv_src_dir}/cy_syslib.c
  ${pdl_drv_src_dir}/cy_gpio.c
  ${pdl_drv_src_dir}/cy_ipc_drv.c
  ${pdl_drv_src_dir}/cy_ipc_pipe.c
  ${pdl_drv_src_dir}/cy_ipc_sema.c
  ${pdl_drv_src_dir}/cy_sysclk_v2.c
  ${pdl_drv_src_dir}/cy_mpc.c
  ${pdl_drv_src_dir}/cy_ppc.c
  ${pdl_drv_src_dir}/cy_syspm_pdcm.c
  ${pdl_drv_src_dir}/cy_syspm_ppu.c
  ${pdl_drv_src_dir}/ppu_v1.c
  ${pdl_drv_src_dir}/cy_ms_ctl.c
  ${pdl_drv_src_dir}/cy_trigmux.c
)
