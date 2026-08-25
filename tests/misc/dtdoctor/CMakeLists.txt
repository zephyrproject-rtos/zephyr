# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(dtdoctor_test)
target_sources(app PRIVATE src/main.c)

enable_testing()
include(CTest)

# Unit tests for the dtdoctor scripts (fixture DTS and Kconfig trees)
add_test(
  NAME dtdoctor_unit
  COMMAND ${PYTHON_EXECUTABLE} -m pytest ${CMAKE_CURRENT_SOURCE_DIR}/unit -v
)
