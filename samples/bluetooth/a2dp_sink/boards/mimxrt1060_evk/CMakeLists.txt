# SPDX-License-Identifier: Apache-2.0

FILE(GLOB app_sources2
  board_codec.c
  board_hardware.c
  ${ZEPHYR_HAL_NXP_MODULE_DIR}/mcux/mcux-sdk/drivers/sai/fsl_sai.c
  ${ZEPHYR_HAL_NXP_MODULE_DIR}/mcux/mcux-sdk/drivers/sai/fsl_sai.h
  ${ZEPHYR_HAL_NXP_MODULE_DIR}/mcux/mcux-sdk/drivers/sai/fsl_sai_edma.c
  ${ZEPHYR_HAL_NXP_MODULE_DIR}/mcux/mcux-sdk/components/i2c/fsl_adapter_lpi2c.c
  ${ZEPHYR_HAL_NXP_MODULE_DIR}/mcux/mcux-sdk/components/codec/fsl_codec_common.c
  ${ZEPHYR_HAL_NXP_MODULE_DIR}/mcux/mcux-sdk/components/codec/i2c/fsl_codec_i2c.c
  ${ZEPHYR_HAL_NXP_MODULE_DIR}/mcux/mcux-sdk/components/codec/port/wm8960/fsl_codec_wm8960_adapter.c
  ${ZEPHYR_HAL_NXP_MODULE_DIR}/mcux/mcux-sdk/components/codec/wm8960/fsl_wm8960.c
  ${ZEPHYR_HAL_NXP_MODULE_DIR}/mcux/mcux-sdk/drivers/edma/fsl_edma.c
  ${ZEPHYR_HAL_NXP_MODULE_DIR}/mcux/mcux-sdk/drivers/dmamux/fsl_dmamux.c
  ${ZEPHYR_HAL_NXP_MODULE_DIR}/mcux/mcux-sdk/drivers/lpi2c/fsl_lpi2c.c
  ${ZEPHYR_HAL_NXP_MODULE_DIR}/mcux/mcux-sdk/components/audio/fsl_adapter_sai.c
  )

target_sources(app PRIVATE ${app_sources2})

zephyr_include_directories(${ZEPHYR_BASE}/samples/bluetooth/a2dp_sink/boards/mimxrt1060_evk)
zephyr_include_directories(${ZEPHYR_HAL_NXP_MODULE_DIR}/mcux/mcux-sdk/drivers/sai)
zephyr_include_directories(${ZEPHYR_HAL_NXP_MODULE_DIR}/mcux/mcux-sdk/components/i2c)
zephyr_include_directories(${ZEPHYR_HAL_NXP_MODULE_DIR}/mcux/mcux-sdk/components/codec)
zephyr_include_directories(${ZEPHYR_HAL_NXP_MODULE_DIR}/mcux/mcux-sdk/components/codec/i2c)
zephyr_include_directories(${ZEPHYR_HAL_NXP_MODULE_DIR}/mcux/mcux-sdk/components/codec/port)
zephyr_include_directories(${ZEPHYR_HAL_NXP_MODULE_DIR}/mcux/mcux-sdk/components/codec/port/wm8960)
zephyr_include_directories(${ZEPHYR_HAL_NXP_MODULE_DIR}/mcux/mcux-sdk/components/codec/wm8960)
zephyr_include_directories(${ZEPHYR_HAL_NXP_MODULE_DIR}/mcux/mcux-sdk/drivers/edma)
zephyr_include_directories(${ZEPHYR_HAL_NXP_MODULE_DIR}/mcux/mcux-sdk/drivers/dmamux)
zephyr_include_directories(${ZEPHYR_HAL_NXP_MODULE_DIR}/mcux/mcux-sdk/drivers/lpi2c)
zephyr_include_directories(${ZEPHYR_HAL_NXP_MODULE_DIR}/mcux/mcux-sdk/components/audio)
