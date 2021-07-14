# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(a2dp_sink)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

#if(CONFIG_BOARD=="mimxrt1060_evk")
  zephyr_library_sources(
    boards/mimxrt1060_evk/fsl_sai.c
    boards/mimxrt1060_evk/board_codec.c
    boards/mimxrt1060_evk/fsl_sai.h
    boards/mimxrt1060_evk/fsl_sai_edma.c
    boards/mimxrt1060_evk/fsl_adapter_lpi2c.c
    boards/mimxrt1060_evk/fsl_codec_common.c
    boards/mimxrt1060_evk/fsl_codec_i2c.c
    boards/mimxrt1060_evk/fsl_wm8960_adapter.c
    boards/mimxrt1060_evk/fsl_wm8960.c
    boards/mimxrt1060_evk/fsl_edma.c
    boards/mimxrt1060_evk/fsl_dmamux.c
    boards/mimxrt1060_evk/fsl_lpi2c.c
    boards/mimxrt1060_evk/fsl_codec_adapter.c
    )
  zephyr_library_include_directories(${ZEPHYR_BASE}/samples/bluetooth/a2dp_sink/boards/mimxrt1060_evk)
#endif()
zephyr_library_include_directories(${ZEPHYR_BASE}/samples/bluetooth)
