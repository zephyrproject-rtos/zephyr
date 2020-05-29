# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(cmsis_dsp_filtering)

FILE(GLOB app_sources src/*.c)

target_sources(app PRIVATE src/main.c)

target_sources_ifdef(
  CONFIG_CMSIS_DSP_TEST_FILTERING_BIQUAD
  app PRIVATE
  src/biquad_q15.c
  src/biquad_q31.c
  src/biquad_f32.c
  src/biquad_f64.c
  )

target_sources_ifdef(
  CONFIG_CMSIS_DSP_TEST_FILTERING_FIR
  app PRIVATE
  src/fir_q7.c
  src/fir_q15.c
  src/fir_q31.c
  src/fir_f32.c
  )

target_sources_ifdef(
  CONFIG_CMSIS_DSP_TEST_FILTERING_MISC
  app PRIVATE
  src/misc_q7.c
  src/misc_q15.c
  src/misc_q31.c
  src/misc_f32.c
  )
