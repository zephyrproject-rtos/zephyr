# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(cmsis_dsp_matrix)

target_sources(app PRIVATE src/main.c)

target_sources_ifdef(
  CONFIG_CMSIS_DSP_TEST_MATRIX_UNARY_Q7
  app PRIVATE src/unary_q7.c
  )

target_sources_ifdef(
  CONFIG_CMSIS_DSP_TEST_MATRIX_UNARY_Q15
  app PRIVATE src/unary_q15.c
  )

target_sources_ifdef(
  CONFIG_CMSIS_DSP_TEST_MATRIX_UNARY_Q31
  app PRIVATE src/unary_q31.c
  )

target_sources_ifdef(
  CONFIG_CMSIS_DSP_TEST_MATRIX_UNARY_F16
  app PRIVATE src/unary_f16.c
  )

target_sources_ifdef(
  CONFIG_CMSIS_DSP_TEST_MATRIX_UNARY_F32
  app PRIVATE src/unary_f32.c
  )

target_sources_ifdef(
  CONFIG_CMSIS_DSP_TEST_MATRIX_UNARY_F64
  app PRIVATE src/unary_f64.c
  )

target_sources_ifdef(
  CONFIG_CMSIS_DSP_TEST_MATRIX_BINARY_Q7
  app PRIVATE src/binary_q7.c
  )

target_sources_ifdef(
  CONFIG_CMSIS_DSP_TEST_MATRIX_BINARY_Q15
  app PRIVATE src/binary_q15.c
  )

target_sources_ifdef(
  CONFIG_CMSIS_DSP_TEST_MATRIX_BINARY_Q31
  app PRIVATE src/binary_q31.c
  )

target_sources_ifdef(
  CONFIG_CMSIS_DSP_TEST_MATRIX_BINARY_F16
  app PRIVATE src/binary_f16.c
  )

target_sources_ifdef(
  CONFIG_CMSIS_DSP_TEST_MATRIX_BINARY_F32
  app PRIVATE src/binary_f32.c
  )

target_sources_ifdef(
  CONFIG_CMSIS_DSP_TEST_MATRIX_BINARY_F64
  app PRIVATE src/binary_f64.c
  )
