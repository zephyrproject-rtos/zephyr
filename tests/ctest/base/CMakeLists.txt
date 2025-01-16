# Copyright (c) 2024 Basalte bv
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(ctest_base)

target_sources(app PRIVATE ${ZEPHYR_BASE}/misc/empty_file.c)

enable_testing()
include(CTest)

add_test(NAME exe_help COMMAND ${CMAKE_BINARY_DIR}/zephyr/zephyr.exe --help)
add_test(NAME exe_run COMMAND ${CMAKE_BINARY_DIR}/zephyr/zephyr.exe -stop_at=3 -no-rt)
set_property(
  TEST exe_run
  PROPERTY PASS_REGULAR_EXPRESSION "Stopped at [0-9.]+s"
)

# A test that always succeeds
add_test(NAME success COMMAND ${CMAKE_COMMAND} -E true)

# A test that is expected to fail
add_test(NAME failure COMMAND ${CMAKE_COMMAND} -E false)
set_property(TEST failure PROPERTY WILL_FAIL true)
