# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

list(APPEND OVERLAY_CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/hci_common.conf)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hci_uart)

target_sources(app PRIVATE src/main.c)
