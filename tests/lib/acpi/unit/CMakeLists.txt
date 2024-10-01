# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

project(lib_acpi)
find_package(Zephyr COMPONENTS unittest REQUIRED HINTS $ENV{ZEPHYR_BASE})

# Assert library
add_library(mock_assert STATIC src/assert.c)
target_link_libraries(mock_assert PRIVATE test_interface)
target_compile_options(test_interface INTERFACE -include ztest.h)

target_include_directories(testbinary PRIVATE
  ${ZEPHYR_BASE}/../modules/lib
  ${ZEPHYR_BASE}/../modules/lib/acpica/source/include
  ${ZEPHYR_BASE}/../modules/lib/acpica/source/tools/acpiexec
  )

target_sources(testbinary PRIVATE
  src/main.c
  src/mock.c
  )

target_link_libraries(testbinary PRIVATE mock_assert)
