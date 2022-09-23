# Copyright (c) 2022 Google LLC
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

set(KCONFIG_ROOT ${CMAKE_CURRENT_LIST_DIR}/../Kconfig)

# Add the sources
list(APPEND SOURCES src/main.cpp)
if(CONFIG_ZTEST_FAIL_TEST_ASSERT_AFTER)
  list(APPEND SOURCES src/assert_after.cpp)
elseif(CONFIG_ZTEST_FAIL_TEST_ASSERT_TEARDOWN)
  list(APPEND SOURCES src/assert_teardown.cpp)
elseif(CONFIG_ZTEST_FAIL_TEST_ASSUME_AFTER)
  list(APPEND SOURCES src/assume_after.cpp)
elseif(CONFIG_ZTEST_FAIL_TEST_ASSUME_TEARDOWN)
  list(APPEND SOURCES src/assume_teardown.cpp)
elseif(CONFIG_ZTEST_FAIL_TEST_PASS_AFTER)
  list(APPEND SOURCES src/pass_after.cpp)
elseif(CONFIG_ZTEST_FAIL_TEST_PASS_TEARDOWN)
  list(APPEND SOURCES src/pass_teardown.cpp)
endif()

if(BOARD STREQUAL unit_testing)
  find_package(Zephyr COMPONENTS unittest REQUIRED HINTS $ENV{ZEPHYR_BASE})
  project(base)

  target_include_directories(testbinary PRIVATE include)
  install(TARGETS testbinary)
else()
  find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
  project(base)

  target_sources(app PRIVATE ${SOURCES})
  target_include_directories(app PRIVATE include)
  install(TARGETS ${logical_target_for_zephyr_elf})
endif()
