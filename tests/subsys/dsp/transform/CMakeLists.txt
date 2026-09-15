# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(zdsp_transform)

target_sources(app PRIVATE
  src/cfft_f32.c
  src/cfft_q15.c
  src/rfft_f32.c
  src/rfft_q15.c
  )

target_include_directories(app PRIVATE ${ZEPHYR_BASE}/tests/lib/cmsis_dsp)

if(COMPILER STREQUAL arcmwdt)
  get_property(Z_ARC_DSP_OPTIONS GLOBAL PROPERTY z_arc_dsp_options)
  target_compile_options(app PRIVATE ${Z_ARC_DSP_OPTIONS})
endif()
