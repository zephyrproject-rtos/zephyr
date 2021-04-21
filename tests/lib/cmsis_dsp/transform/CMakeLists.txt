# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(cmsis_dsp_transform)

FILE(GLOB app_sources src/*.c)

target_sources(app PRIVATE src/main.c)

target_sources_ifdef(
  CONFIG_CMSIS_DSP_TEST_TRANSFORM_CQ15
  app PRIVATE src/cq15.c
  )

target_sources_ifdef(
  CONFIG_CMSIS_DSP_TEST_TRANSFORM_RQ15
  app PRIVATE src/rq15.c
  )

target_sources_ifdef(
  CONFIG_CMSIS_DSP_TEST_TRANSFORM_CQ31
  app PRIVATE src/cq31.c
  )

target_sources_ifdef(
  CONFIG_CMSIS_DSP_TEST_TRANSFORM_RQ31
  app PRIVATE src/rq31.c
  )

target_sources_ifdef(
  CONFIG_CMSIS_DSP_TEST_TRANSFORM_CF32
  app PRIVATE src/cf32.c
  )

target_sources_ifdef(
  CONFIG_CMSIS_DSP_TEST_TRANSFORM_RF32
  app PRIVATE src/rf32.c
  )

target_sources_ifdef(
  CONFIG_CMSIS_DSP_TEST_TRANSFORM_CF64
  app PRIVATE src/cf64.c
  )

target_sources_ifdef(
  CONFIG_CMSIS_DSP_TEST_TRANSFORM_RF64
  app PRIVATE src/rf64.c
  )
