# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr COMPONENTS unittest HINTS $ENV{ZEPHYR_BASE})
project(bt_keys_get_addr)

add_subdirectory(${ZEPHYR_BASE}/tests/bluetooth/host host_mocks)
add_subdirectory(${ZEPHYR_BASE}/tests/bluetooth/host/keys/mocks mocks)

target_link_libraries(testbinary PRIVATE mocks host_mocks)

target_sources(testbinary
    PRIVATE
    src/main.c
    src/test_suite_full_list_invalid_values.c
    src/test_suite_full_list_no_overwrite.c
    src/test_suite_full_list_overwrite_oldest.c
    $ENV{ZEPHYR_BASE}/subsys/bluetooth/common/bt_str.c
    $ENV{ZEPHYR_BASE}/subsys/bluetooth/host/uuid.c

    # Unit under test
    $ENV{ZEPHYR_BASE}/subsys/bluetooth/host/keys.c
    $ENV{ZEPHYR_BASE}/subsys/bluetooth/common/addr.c
)
