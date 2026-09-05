# SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
# SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

set(pdl_dir ${ZEPHYR_HAL_INFINEON_MODULE_DIR}/mtb-dsl-psc3m6/pdl)

set(pdl_drv_dir ${pdl_dir}/drivers)
set(pdl_dev_psc3m6_dir ${pdl_dir}/devices)

# Generate PDL specific SOC defines
zephyr_compile_definitions($<UPPER_CASE:${CONFIG_SOC}>)
zephyr_compile_definitions(CY_SRF_DISABLE)
zephyr_library_compile_definitions($<UPPER_CASE:${CONFIG_SOC}>)

# Add device headers
zephyr_include_directories(${pdl_drv_dir}/include)
zephyr_include_directories(${pdl_dev_psc3m6_dir}/include)
zephyr_include_directories(${pdl_dev_psc3m6_dir}/include/ip)
zephyr_library_sources(${pdl_dev_psc3m6_dir}/source/cy_device.c)

if(${ZEPHYR_TOOLCHAIN_VARIANT} STREQUAL "armclang")
  zephyr_library_sources(${pdl_drv_dir}/source/TOOLCHAIN_ARM/cy_syslib_ext.S)
else()
  zephyr_library_sources(${pdl_drv_dir}/source/TOOLCHAIN_GCC_ARM/cy_syslib_ext.S)
endif()

# Peripheral drivers
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_FLASH ${pdl_drv_dir}/source/cy_flash.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_I2C ${pdl_drv_dir}/source/cy_scb_i2c.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_LPTIMER ${pdl_drv_dir}/source/cy_mcwdt.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_PWM ${pdl_drv_dir}/source/cy_tcpwm_pwm.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_RTC ${pdl_drv_dir}/source/cy_rtc.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_SPI ${pdl_drv_dir}/source/cy_scb_spi.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_TIMER ${pdl_drv_dir}/source/cy_tcpwm_counter.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_UART ${pdl_drv_dir}/source/cy_scb_uart.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_WDT ${pdl_drv_dir}/source/cy_wdt.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_LPCOMP ${pdl_drv_dir}/source/cy_lpcomp.c)

if(CONFIG_USE_INFINEON_UART OR CONFIG_USE_INFINEON_I2C OR CONFIG_USE_INFINEON_SPI)
  zephyr_library_sources(${pdl_drv_dir}/source/cy_scb_common.c)
endif()

zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_DMA ${pdl_drv_dir}/source/cy_dma.c)

# HPPASS
zephyr_library_sources(${pdl_drv_dir}/source/cy_hppass.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_hppass_ac.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_hppass_csg.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_hppass_sar.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_hppass_hwfilt3p3z.c)

# Common part
zephyr_library_sources(${pdl_drv_dir}/source/cy_syspm_v2.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_syslib.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_gpio.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_ipc_drv.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_ipc_pipe.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_ipc_sema.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_systick_v2.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_sysclk_v2.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_mpc.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_ppc.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_syspm_pdcm.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_syspm_ppu.c)
zephyr_library_sources(${pdl_drv_dir}/source/ppu_v1.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_ms_ctl.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_trigmux.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_sysint_v2.c)
