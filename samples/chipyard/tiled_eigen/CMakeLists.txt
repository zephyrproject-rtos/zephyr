# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tiled_eigen)

target_include_directories(app PRIVATE . ${CMAKE_CURRENT_SOURCE_DIR}/../../../../modules/lib/eigen)

target_compile_definitions(app PRIVATE CONFIG_MATRIX_SIZE=128)

target_sources(app PRIVATE src/main.cpp)
