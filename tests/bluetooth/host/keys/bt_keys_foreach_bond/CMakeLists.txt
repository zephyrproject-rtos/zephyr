# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr COMPONENTS unittest REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(bt_keys_foreach_bond)

add_subdirectory(${ZEPHYR_BASE}/tests/bluetooth/host host_mocks)
add_subdirectory(${ZEPHYR_BASE}/tests/bluetooth/host/keys/mocks mocks)

target_link_libraries(testbinary PRIVATE mocks host_mocks)

target_sources(testbinary
    PRIVATE
    src/main.c
    src/test_suite_foreach_bond_invalid_inputs.c

    # Unit under test
    ${ZEPHYR_BASE}/subsys/bluetooth/host/keys.c
    ${ZEPHYR_BASE}/subsys/bluetooth/common/addr.c
)
