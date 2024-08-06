# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr COMPONENTS unittest HINTS $ENV{ZEPHYR_BASE})

project(bt_pub_key_gen)

add_subdirectory(${ZEPHYR_BASE}/tests/bluetooth/host host_mocks)
add_subdirectory(${ZEPHYR_BASE}/tests/bluetooth/host/ecc mocks)

target_sources(testbinary PRIVATE
  src/main.c
  src/test_suite_invalid_inputs.c
)
target_link_libraries(testbinary PRIVATE mocks host_mocks)
