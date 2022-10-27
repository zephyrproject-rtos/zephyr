# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

set(ZEPHYR_MODULES "${CMAKE_CURRENT_SOURCE_DIR}/my_module")
set(ZEPHYR_EXTRA_MODULES "${CMAKE_CURRENT_SOURCE_DIR}/my_extra_module")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(overlay_var_expansions)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
