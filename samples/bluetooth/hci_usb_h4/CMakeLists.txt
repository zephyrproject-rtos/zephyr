# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

list(APPEND OVERLAY_CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/../hci_uart/hci_common.conf)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(hci_usb_h4)

target_sources(app PRIVATE src/main.c)
