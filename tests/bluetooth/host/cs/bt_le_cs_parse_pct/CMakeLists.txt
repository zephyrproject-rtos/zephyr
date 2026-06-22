# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr COMPONENTS unittest REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(bt_step_data_parse)

include_directories(BEFORE
    ${ZEPHYR_BASE}/tests/bluetooth/host/cs/mocks
)

add_subdirectory(${ZEPHYR_BASE}/tests/bluetooth/host host_mocks)
add_subdirectory(${ZEPHYR_BASE}/tests/bluetooth/host/cs/mocks mocks)

target_link_libraries(testbinary PRIVATE mocks host_mocks)

target_sources(testbinary
    PRIVATE
    src/main.c

    ${ZEPHYR_BASE}/subsys/bluetooth/host/cs.c
    ${ZEPHYR_BASE}/lib/net_buf/buf_simple.c
    ${ZEPHYR_BASE}/subsys/logging/log_minimal.c
)
