# SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
# SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(dual_board_loopback_controller)

file(GLOB app_sources CONFIGURE_DEPENDS src/*.c)
list(REMOVE_ITEM app_sources "${CMAKE_CURRENT_SOURCE_DIR}/src/test_harness.c")
target_sources(app PRIVATE ${app_sources})
target_sources_ifdef(CONFIG_I3C_DUAL_BOARD_LOOPBACK_META_TESTS app PRIVATE src/test_harness.c)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../common)
