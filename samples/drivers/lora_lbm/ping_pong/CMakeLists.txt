# Copyright (c) 2024 Semtech Corporation - Félix Piédallu
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})
project(ping_pong)

zephyr_compile_options(-fdiagnostics-color=always)


zephyr_include_directories(.)
# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../common src/common)
add_subdirectory(src)
