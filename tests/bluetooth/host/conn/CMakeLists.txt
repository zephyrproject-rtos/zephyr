# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr COMPONENTS unittest REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(conn)

include_directories(BEFORE
    ${ZEPHYR_BASE}/tests/bluetooth/host/conn
    ${ZEPHYR_BASE}/tests/bluetooth/host/conn/mocks/zephyr/include
)

add_subdirectory(${ZEPHYR_BASE}/tests/bluetooth/host host_mocks)
add_subdirectory(${ZEPHYR_BASE}/tests/bluetooth/host/conn/mocks mocks)

target_link_libraries(testbinary PRIVATE mocks host_mocks)

target_sources(testbinary
    PRIVATE
    src/main.c

    ${ZEPHYR_BASE}/subsys/bluetooth/host/conn.c
    ${ZEPHYR_BASE}/subsys/logging/log_minimal.c
    ${ZEPHYR_BASE}/lib/net_buf/buf.c
    ${ZEPHYR_BASE}/lib/net_buf/buf_simple.c
)
