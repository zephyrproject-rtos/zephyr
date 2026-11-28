# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(arcmwdt_relocation)

add_custom_target(check_arcmwdt_relocation
  COMMAND ${CMAKE_COMMAND}
    -DZEPHYR_BASE=${ZEPHYR_BASE}
    -DPYTHON_EXECUTABLE=${PYTHON_EXECUTABLE}
    -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
    -DGENERATOR=${CMAKE_GENERATOR}
    -DCMAKE_VERBOSE_MAKEFILE=${CMAKE_VERBOSE_MAKEFILE}
    -DBUILD_DIR=${CMAKE_CURRENT_BINARY_DIR}/relocation-fixture
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_relocation.cmake
  VERBATIM
)
target_sources(app PRIVATE ${ZEPHYR_BASE}/misc/empty_file.c)
add_dependencies(app check_arcmwdt_relocation)
