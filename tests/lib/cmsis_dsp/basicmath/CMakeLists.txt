# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(cmsis_dsp_basicmath)

target_sources(app PRIVATE
  src/q7.c
  src/q15.c
  src/q31.c
  src/f32.c
  )

target_sources_ifdef(CONFIG_CMSIS_DSP_FLOAT16 app PRIVATE src/f16.c)
