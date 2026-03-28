# SPDX-FileCopyrightText: Copyright Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_usbh)

include(${ZEPHYR_BASE}/samples/subsys/usb/common/common.cmake)

target_include_directories(app PRIVATE include)
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/tests/subsys/usb/host/common)
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/usb/host)

target_sources(app PRIVATE src/main.c src/class.c)
