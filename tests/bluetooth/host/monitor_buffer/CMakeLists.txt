# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr COMPONENTS unittest HINTS $ENV{ZEPHYR_BASE})

project(bt_monitor_buffer)

add_subdirectory(${ZEPHYR_BASE}/tests/bluetooth/host host_mocks)

target_include_directories(testbinary PRIVATE ${ZEPHYR_BASE}/subsys/bluetooth/host)
target_sources(testbinary PRIVATE src/main.c ${ZEPHYR_BASE}/lib/utils/ring_buffer.c)
target_link_libraries(testbinary PRIVATE host_mocks)
