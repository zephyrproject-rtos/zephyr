# Copyright (c) 2021-2025 Gerson Fernando Budke <nandojve@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_FAMILY_BOUFFALOLAB_BFLB)
  zephyr_library_named(hal_bouffalolab)

  zephyr_library_compile_definitions(
    BFLB_USE_HAL_DRIVER
    BFLB_USE_CUSTOM_LD_SECTIONS
  )

  set(bflb_soc         bl602)
  set(bflb_drv_dir     ${ZEPHYR_HAL_BOUFFALOLAB_MODULE_DIR}/drivers/${bflb_soc}_driver)
  set(bflb_common_dir  ${ZEPHYR_HAL_BOUFFALOLAB_MODULE_DIR}/common)
  set(bflb_drv_src_dir ${bflb_drv_dir}/std_drv/src)

  # Global includes
  zephyr_include_directories(
    include
    ${ZEPHYR_HAL_BOUFFALOLAB_MODULE_DIR}/include

    ${bflb_drv_dir}/regs
    ${bflb_drv_dir}/startup
    ${bflb_drv_dir}/std_drv/inc

    ${bflb_common_dir}/misc
  )

  zephyr_library_include_directories(
    ${bflb_common_dir}/soft_crc
  )

  zephyr_library_sources(
    ${bflb_drv_src_dir}/${bflb_soc}_aon.c
    ${bflb_drv_src_dir}/${bflb_soc}_ef_ctrl.c
    ${bflb_drv_src_dir}/${bflb_soc}_glb.c
    ${bflb_drv_src_dir}/${bflb_soc}_hbn.c
    ${bflb_drv_src_dir}/${bflb_soc}_l1c.c
    ${bflb_drv_src_dir}/${bflb_soc}_pds.c
    ${bflb_drv_src_dir}/${bflb_soc}_romapi.c

    ${bflb_common_dir}/soft_crc/softcrc.c
  )

  zephyr_library_sources_ifdef(CONFIG_USE_BFLB_ACOMP ${bflb_drv_src_dir}/${bflb_soc}_acomp.c)
  zephyr_library_sources_ifdef(CONFIG_USE_BFLB_ADC   ${bflb_drv_src_dir}/${bflb_soc}_adc.c)
  zephyr_library_sources_ifdef(CONFIG_USE_BFLB_DAC   ${bflb_drv_src_dir}/${bflb_soc}_dac.c)
  zephyr_library_sources_ifdef(CONFIG_USE_BFLB_DMA   ${bflb_drv_src_dir}/${bflb_soc}_dma.c)
  zephyr_library_sources_ifdef(CONFIG_USE_BFLB_I2C   ${bflb_drv_src_dir}/${bflb_soc}_i2c.c)
  zephyr_library_sources_ifdef(CONFIG_USE_BFLB_IR    ${bflb_drv_src_dir}/${bflb_soc}_ir.c)
  zephyr_library_sources_ifdef(CONFIG_USE_BFLB_PWM   ${bflb_drv_src_dir}/${bflb_soc}_pwm.c)
  zephyr_library_sources_ifdef(CONFIG_USE_BFLB_SPI   ${bflb_drv_src_dir}/${bflb_soc}_spi.c)
  zephyr_library_sources_ifdef(CONFIG_USE_BFLB_UART  ${bflb_drv_src_dir}/${bflb_soc}_uart.c)
endif() # SOC_FAMILY_BOUFFALOLAB_BFLB
