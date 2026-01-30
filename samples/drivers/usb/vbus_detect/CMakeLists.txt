# Copyright (c) 2026 Leica Geosystems AG
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(usb_vbus_detect_sample)

target_sources(app PRIVATE src/main.c)
