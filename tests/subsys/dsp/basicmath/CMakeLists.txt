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

target_sources_ifdef(CONFIG_FP16 app PRIVATE src/f16.c)

target_include_directories(app PRIVATE ${ZEPHYR_BASE}/tests/lib/cmsis_dsp)

if(COMPILER STREQUAL arcmwdt)
  get_property(Z_ARC_DSP_OPTIONS GLOBAL PROPERTY z_arc_dsp_options)
  target_compile_options(app PRIVATE ${Z_ARC_DSP_OPTIONS})
endif()
