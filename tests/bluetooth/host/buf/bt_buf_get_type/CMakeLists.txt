# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

if (NOT BOARD STREQUAL unit_testing)
  message(FATAL_ERROR "This project can only be used with '-DBOARD=unit_testing'.")
endif()

FILE(GLOB SOURCES src/*.c)

project(bluetooth_bt_buf_get_type)

find_package(Zephyr COMPONENTS unittest HINTS $ENV{ZEPHYR_BASE})

include(${ZEPHYR_BASE}/tests/bluetooth/host/cmake.txt)
include(${ZEPHYR_BASE}/tests/bluetooth/host/buf/cmake.txt)

target_sources(testbinary PRIVATE ${host_module} ${module_mocks} ${host_mocks})
