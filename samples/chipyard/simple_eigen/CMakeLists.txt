# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(simple_eigen)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../modules/lib/eigen)

target_sources(app PRIVATE src/main.cpp)
