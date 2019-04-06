# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(mesh)

zephyr_library_include_directories($ENV{ZEPHYR_BASE}/samples/bluetooth)
target_sources(app PRIVATE src/main.c)
if(CONFIG_BOARD_BBC_MICROBIT)
target_sources(app PRIVATE src/microbit.c)
endif()
