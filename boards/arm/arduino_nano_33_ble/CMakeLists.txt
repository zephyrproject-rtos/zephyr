# Copyright (c) 2020 Jefferson Lee.
# SPDX-License-Identifier: Apache-2.0

add_library(arduino_nano_33_ble_pins INTERFACE)
target_include_directories(arduino_nano_33_ble_pins
        INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/inc/")


zephyr_library()
zephyr_library_sources(board.c)
