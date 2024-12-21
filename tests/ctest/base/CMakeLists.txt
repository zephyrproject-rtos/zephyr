# Copyright (c) 2024 Basalte bv
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(ctest_base)

target_sources(app PRIVATE ${ZEPHYR_BASE}/misc/empty_file.c)

enable_testing()
include(CTest)

# A test that always succeeds
add_test(NAME success COMMAND ${CMAKE_COMMAND} -E true)

# A test that is expected to fail
add_test(NAME failure COMMAND ${CMAKE_COMMAND} -E false)
set_property(TEST failure PROPERTY WILL_FAIL true)
