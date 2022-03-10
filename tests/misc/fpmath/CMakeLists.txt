# Copyright (c) 2022 Google Inc
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(fpmath)

target_include_directories(app PRIVATE include)

if(CONFIG_CPLUSPLUS)
  configure_file(src/q1_0_31.c ${CMAKE_BINARY_DIR}/q1_0_31.cpp COPYONLY)
  configure_file(src/q1_15_16.c ${CMAKE_BINARY_DIR}/q1_15_16.cpp COPYONLY)
  configure_file(src/q1_23_8.c ${CMAKE_BINARY_DIR}/q1_23_8.cpp COPYONLY)
  target_sources(app
    PRIVATE
      src/common.c
      ${CMAKE_BINARY_DIR}/q1_0_31.cpp
      ${CMAKE_BINARY_DIR}/q1_15_16.cpp
      ${CMAKE_BINARY_DIR}/q1_23_8.cpp
  )
else()
  target_sources(app
    PRIVATE
      src/common.c
      src/q1_23_8.c
      src/q1_15_16.c
      src/q1_0_31.c
  )
endif()
