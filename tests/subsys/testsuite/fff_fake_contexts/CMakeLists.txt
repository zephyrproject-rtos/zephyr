# Copyright (c) 2023 Legrand North America, LLC.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
if(BOARD STREQUAL unit_testing)
  find_package(Zephyr COMPONENTS unittest REQUIRED HINTS $ENV{ZEPHYR_BASE})
  set(target testbinary)
else()
  find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
  set(target app)
endif()

project(base)
target_sources(${target} PRIVATE
  src/code_under_test.c
  src/main.c
  src/fakes/called_API.c
)

target_include_directories(${target} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)
