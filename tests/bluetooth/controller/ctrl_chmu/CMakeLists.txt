# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

FILE(GLOB SOURCES
  src/*.c
)

project(bluetooth_ctrl_ull_conn)
find_package(Zephyr COMPONENTS unittest HINTS $ENV{ZEPHYR_BASE})
include(${ZEPHYR_BASE}/tests/bluetooth/controller/common/defaults_cmake.txt)
target_sources(testbinary PRIVATE ${ll_sw_sources} ${mock_sources} ${common_sources})
