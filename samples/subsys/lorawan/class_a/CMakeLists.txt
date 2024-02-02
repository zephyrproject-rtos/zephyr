# SPDX-License-Identifier: Apache-2.0
list(APPEND BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lorawan)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
