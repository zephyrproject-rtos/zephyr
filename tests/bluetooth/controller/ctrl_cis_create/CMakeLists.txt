# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

project(bluetooth_ull_llcp_cis_create)
find_package(Zephyr COMPONENTS unittest REQUIRED HINTS $ENV{ZEPHYR_BASE})

add_subdirectory(${ZEPHYR_BASE}/tests/bluetooth/controller/common common)
add_subdirectory(${ZEPHYR_BASE}/tests/bluetooth/controller/uut uut)

target_link_libraries(testbinary PRIVATE uut common)

target_sources(testbinary
  PRIVATE
    src/main.c
)
