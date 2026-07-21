# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(dsp_conversions)

target_sources(app PRIVATE
  src/q7.c
  src/q15.c
  src/q31.c
  src/f32.c
  src/f64.c
  )

target_include_directories(app PRIVATE ${ZEPHYR_BASE}/tests/lib/cmsis_dsp)
