# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

list(APPEND DTS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../common/fake_hci)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(host_adv_sid)

target_sources(app PRIVATE src/main.c)

add_subdirectory(${ZEPHYR_BASE}/tests/bluetooth/common/fake_hci fake_hci)
target_link_libraries(app PRIVATE fake_hci)
