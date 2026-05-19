# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(usbc_drp)

target_sources(app PRIVATE src/main.c)

# Include common USB-C sample code
include(${CMAKE_CURRENT_SOURCE_DIR}/../common/common.cmake)
