# Copyright (c) 2026 Tolt Technologies
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_usb_cdc_eth)

target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/usb/host)

target_sources(app PRIVATE src/main.c)
