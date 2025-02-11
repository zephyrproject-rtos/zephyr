# Copyright (c) 2022 Google LLC
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
include(ExternalProject)

# Add the sources and set up the build for either unit testing or native_sim
list(APPEND SOURCES src/main.cpp)
if(BOARD STREQUAL unit_testing)
  find_package(Zephyr COMPONENTS unittest REQUIRED HINTS $ENV{ZEPHYR_BASE})
  set(target testbinary)
  # Set the target binary for the 'core' external project. The path to this must match the one set
  # below in ExternalProject_Add's CMAKE_INSTALL_PREFIX
  add_compile_definitions(FAIL_TARGET_BINARY="${CMAKE_BINARY_DIR}/core/bin/testbinary")
else()
  find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
  set(target app)
  # Set the target binary for the 'core' external project. The path to this must match the one set
  # below in ExternalProject_Add's CMAKE_INSTALL_PREFIX
  add_compile_definitions(FAIL_TARGET_BINARY="${CMAKE_BINARY_DIR}/core/bin/zephyr.exe")
endif()

# Create the project and set the sources for the target
project(fail)
target_sources(${target} PRIVATE ${SOURCES})

# Find which CONFIG_ZTEST_FAIL_TEST_* choice was set so we can pass it to the external project
# Once we find the config, we'll need to prepend a '-D' and append '=y' so we can pass it to the
# 'core' project as a cmake argument.
get_cmake_property(_vars VARIABLES)
string(REGEX MATCHALL "(^|;)CONFIG_ZTEST_FAIL_TEST_[A-Za-z0-9_]+" fail_test_config "${_vars}")
list(FILTER fail_test_config EXCLUDE REGEX "^$")
list(TRANSFORM fail_test_config PREPEND "-D")
list(TRANSFORM fail_test_config APPEND "=y")
string(REPLACE ";" " " fail_test_config "${fail_test_config}")

# Add the 'core' external project which will mirror the configs of this project.
ExternalProject_Add(core
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/core
    CMAKE_ARGS
      -DBOARD:STRING=${BOARD}
      -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_BINARY_DIR}/core
      ${fail_test_config}
)
add_dependencies(${target} core)
