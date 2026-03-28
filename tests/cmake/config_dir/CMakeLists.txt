# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

if(DEFINED APPLICATION_CONFIG_DIR)
  # Has been specified as relative on CMake invocation, convert to absolute
  # and keep reference to see it's identical when Zephyr boilerplate returns.
  get_filename_component(EXPECTED_APPLICATION_CONFIG_DIR ${APPLICATION_CONFIG_DIR} ABSOLUTE)
else()
  # We expect a board config setup
  set(EXPECTED_APPLICATION_CONFIG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/boards/${BOARD})
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(app_config_dir_test)

if(CONFIG_FAIL_TEST)
  message(FATAL_ERROR "Test failed, "
    "actual CONFIG_FAIL_TEST=${CONFIG_FAIL_TEST}, "
    "expected CONFIG_FAIL_TEST=n"
  )
endif()

if(NOT "${APPLICATION_CONFIG_DIR}" STREQUAL "${EXPECTED_APPLICATION_CONFIG_DIR}")
  message(FATAL_ERROR "Test failed, "
    "actual APPLICATION_CONFIG_DIR=${APPLICATION_CONFIG_DIR}, "
    "expected APPLICATION_CONFIG_DIR=${EXPECTED_APPLICATION_CONFIG_DIR}"
  )
endif()

target_sources(app PRIVATE src/main.c)
