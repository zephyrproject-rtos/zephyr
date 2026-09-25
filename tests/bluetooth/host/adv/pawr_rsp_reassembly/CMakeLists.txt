# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr COMPONENTS unittest REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(bt_pawr_rsp_reassembly)

add_subdirectory(${ZEPHYR_BASE}/tests/bluetooth/host host_mocks)

target_link_libraries(testbinary PRIVATE host_mocks)

target_sources(testbinary
    PRIVATE
    src/main.c
    src/stubs.c

    ${ZEPHYR_BASE}/subsys/bluetooth/host/adv.c
    ${ZEPHYR_BASE}/subsys/bluetooth/common/addr.c
    ${ZEPHYR_BASE}/subsys/bluetooth/common/bt_str.c
    ${ZEPHYR_BASE}/subsys/bluetooth/host/uuid.c
    ${ZEPHYR_BASE}/lib/net_buf/buf_simple.c
    ${ZEPHYR_BASE}/subsys/logging/log_minimal.c
    ${ZEPHYR_BASE}/lib/utils/hex.c
    ${ZEPHYR_BASE}/lib/uuid/uuid.c
)
