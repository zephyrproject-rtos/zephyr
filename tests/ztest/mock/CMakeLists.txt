# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
if(BOARD STREQUAL unit_testing)
  list(APPEND SOURCES src/main.c)

  include($ENV{ZEPHYR_BASE}/subsys/testsuite/unittest.cmake)
  project(mock)
else()
  include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
  project(mock)

  FILE(GLOB app_sources src/*.c)
  target_sources(app PRIVATE ${app_sources})
endif()
