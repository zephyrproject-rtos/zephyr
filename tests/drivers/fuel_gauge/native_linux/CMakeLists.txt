# SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

# The overlay carries the absolute path of the fake power supply.
configure_file(app.overlay.in ${CMAKE_CURRENT_BINARY_DIR}/app.overlay @ONLY)
set(EXTRA_DTC_OVERLAY_FILE ${CMAKE_CURRENT_BINARY_DIR}/app.overlay)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(native_linux_fuel_gauge)

target_sources(app PRIVATE src/main.c)
