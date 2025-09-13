# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(secondary_boot_secondary)

# Create marker file to identify this as secondary firmware
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/is_secondary_firmware.txt "")

target_sources(app PRIVATE src/main.c)
