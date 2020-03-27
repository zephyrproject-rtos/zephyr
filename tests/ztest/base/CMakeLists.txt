# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
if(BOARD STREQUAL unit_testing)
  list(APPEND SOURCES src/main.c)

  include($ENV{ZEPHYR_BASE}/subsys/testsuite/unittest.cmake)
  project(base)
else()
  find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})
  project(base)

  FILE(GLOB app_sources src/*.c)
  target_sources(app PRIVATE ${app_sources})
endif()
