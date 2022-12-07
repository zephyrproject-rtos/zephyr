# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

project(bluetooth_bt_buf_get_evt)

find_package(Zephyr COMPONENTS unittest HINTS $ENV{ZEPHYR_BASE})

add_subdirectory(${ZEPHYR_BASE}/tests/bluetooth/host host_mocks)
add_subdirectory(${ZEPHYR_BASE}/tests/bluetooth/host/buf mocks)

target_link_libraries(testbinary PRIVATE mocks host_mocks)
target_sources(testbinary
  PRIVATE
    src/main.c
    src/test_suite_hci_evt_cmd.c
    src/test_suite_hci_evt_num_completed_packets.c
)
