# Copyright (c) 2022 Google Inc
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(fpmath)
target_sources(app
  PRIVATE
    src/main.cpp
)
target_sources_ifdef(CONFIG_MATH_UTIL_FLOAT app
  PRIVATE
    src/float_tests.cpp
)
target_sources_ifndef(CONFIG_MATH_UTIL_FLOAT app
  PRIVATE
    "src/fixed_tests_${CONFIG_MATH_UTIL_FP_BITS}bit.cpp"
)
