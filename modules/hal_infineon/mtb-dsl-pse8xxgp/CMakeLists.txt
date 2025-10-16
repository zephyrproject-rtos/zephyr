# Copyright (c) 2025 Infineon Technologies AG,
# or an affiliate of Infineon Technologies AG.
#
# SPDX-License-Identifier: Apache-2.0

set(pdl_dir ${ZEPHYR_HAL_INFINEON_MODULE_DIR}/mtb-dsl-pse8xxgp/pdl)
set(hal_dir ${ZEPHYR_HAL_INFINEON_MODULE_DIR}/mtb-dsl-pse8xxgp/hal)

set(pdl_drv_dir ${pdl_dir}/drivers)
set(pdl_dev_edge_dir ${pdl_dir}/devices)

# Generate PDL specific SOC defines
zephyr_library_compile_definitions($<UPPER_CASE:${CONFIG_SOC}>)

# Add mtb-pdl-cat1
zephyr_include_directories(${pdl_drv_dir}/include)
zephyr_include_directories(${pdl_drv_dir}/third_party/ethernet/include)

zephyr_include_directories(${pdl_dev_edge_dir}/include)
zephyr_include_directories(${pdl_dev_edge_dir}/include/ip)
zephyr_library_sources(${pdl_dev_edge_dir}/source/cy_device.c)

if(${ZEPHYR_TOOLCHAIN_VARIANT} STREQUAL "armclang")
  zephyr_library_sources(${pdl_drv_dir}/source/TOOLCHAIN_ARM/cy_syslib_ext.S)
else()
  zephyr_library_sources(${pdl_drv_dir}/source/TOOLCHAIN_GCC_ARM/cy_syslib_ext.S)
endif()

# Peripheral drivers
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_I2C ${pdl_drv_dir}/source/cy_scb_i2c.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_I3C ${pdl_drv_dir}/source/cy_i3c.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_LPTIMER ${pdl_drv_dir}/source/cy_mcwdt.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_PWM ${pdl_drv_dir}/source/cy_tcpwm_pwm.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_RTC ${pdl_drv_dir}/source/cy_rtc.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_SDIO ${pdl_drv_dir}/source/cy_sd_host.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_SPI ${pdl_drv_dir}/source/cy_scb_spi.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_TIMER ${pdl_drv_dir}/source/cy_tcpwm_counter.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_UART ${pdl_drv_dir}/source/cy_scb_uart.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_WDT ${pdl_drv_dir}/source/cy_wdt.c)

zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_AUTANALOG_SAR_ADC ${pdl_drv_dir}/source/cy_systrimm.c)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_AUTANALOG_SAR_ADC ${pdl_drv_dir}/source/cy_rram.c)

if(CONFIG_USE_INFINEON_TRNG)
  zephyr_library_sources(${pdl_drv_dir}/source/cy_crypto.c)
  zephyr_library_sources(${pdl_drv_dir}/source/cy_crypto_core_trng_v1.c)
  zephyr_library_sources(${pdl_drv_dir}/source/cy_crypto_core_trng_v2.c)
endif()

if(CONFIG_USE_INFINEON_UART OR CONFIG_USE_INFINEON_I2C OR CONFIG_USE_INFINEON_SPI)
  zephyr_library_sources(${pdl_drv_dir}/source/cy_scb_common.c)
endif()

if(CONFIG_USE_INFINEON_DMA OR CONFIG_USE_INFINEON_SMIF)
  zephyr_library_sources(${pdl_drv_dir}/source/cy_dma.c)
  zephyr_library_sources(${pdl_drv_dir}/source/cy_dmac.c)
endif()

zephyr_library_sources(${pdl_drv_dir}/source/cy_syspm_v4.c)
zephyr_code_relocate(FILES ${pdl_drv_dir}/source/cy_syspm_v4.c LOCATION RAM)

zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_SMIF ${pdl_drv_dir}/source/cy_smif.c)
zephyr_code_relocate(FILES ${pdl_drv_dir}/source/cy_smif.c LOCATION RAM)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_SMIF ${pdl_drv_dir}/source/cy_smif_sfdp.c)
zephyr_code_relocate(FILES ${pdl_drv_dir}/source/cy_smif_sfdp.c LOCATION RAM)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_SMIF ${pdl_drv_dir}/source/cy_smif_memslot.c)
zephyr_code_relocate(FILES ${pdl_drv_dir}/source/cy_smif_memslot.c LOCATION RAM)
zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_SMIF ${pdl_drv_dir}/source/cy_smif_hb_flash.c)
zephyr_code_relocate(FILES ${pdl_drv_dir}/source/cy_smif_hb_flash.c LOCATION RAM)

zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_SMIF ${pdl_drv_dir}/source/cy_smif_memnum.c)
zephyr_code_relocate(FILES ${pdl_drv_dir}/source/cy_smif_memnum.c LOCATION RAM)

zephyr_library_sources(${pdl_drv_dir}/source/cy_syslib.c)
zephyr_code_relocate(FILES ${pdl_drv_dir}/source/cy_syslib.c LOCATION RAM)

zephyr_library_sources(${pdl_drv_dir}/source/cy_autanalog.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_autanalog_sar.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_autanalog_ac.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_autanalog_ctb.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_autanalog_ptc.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_autanalog_dac.c)

# Common part
zephyr_library_sources(${pdl_drv_dir}/source/cy_gpio.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_ipc_drv.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_ipc_pipe.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_ipc_sema.c)

zephyr_library_sources(${pdl_drv_dir}/source/cy_sysclk_v2.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_mpc.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_ppc.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_syspm_pdcm.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_syspm_ppu.c)
zephyr_library_sources(${pdl_drv_dir}/source/ppu_v1.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_smif.c)

zephyr_library_sources(${pdl_drv_dir}/source/cy_trigmux.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_wdt.c)
zephyr_library_sources(${pdl_drv_dir}/source/cy_tcpwm_pwm.c)

zephyr_include_directories(${hal_dir}/include)
zephyr_library_sources(${hal_dir}/source/mtb_hal_clock.c)
