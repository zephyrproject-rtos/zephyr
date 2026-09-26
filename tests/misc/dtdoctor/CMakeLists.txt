# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

# The e2e suite replays the application's own compile commands through the
# SCA wrapper, against deliberately-broken translation units
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(dtdoctor_test)
target_sources(app PRIVATE src/main.c src/main.cpp)

enable_testing()
include(CTest)

# Unit tests for the dtdoctor scripts (fixture DTS and Kconfig trees)
add_test(
  NAME dtdoctor_unit
  COMMAND ${PYTHON_EXECUTABLE} -m pytest ${CMAKE_CURRENT_SOURCE_DIR}/unit -v
)

add_test(
  NAME dtdoctor_e2e
  COMMAND ${PYTHON_EXECUTABLE} -m pytest
    ${CMAKE_CURRENT_SOURCE_DIR}/e2e
    --build-dir ${CMAKE_BINARY_DIR}
    --cc ${CMAKE_C_COMPILER}
    -v
)

# Reproduce the Kconfig environment the dtdoctor SCA launcher runs with, so the
# analyzer can parse the full Zephyr Kconfig tree for the enabled-node diagnosis
set_tests_properties(
  dtdoctor_e2e
  PROPERTIES ENVIRONMENT "${COMMON_KCONFIG_ENV_SETTINGS}"
)
