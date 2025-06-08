# Sierra Wireless HL78XX Sample CMake file

# Copyright (c) 2025 Netfeasa
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hello_hl78xx)

target_sources(app PRIVATE src/main.c)

include(${ZEPHYR_BASE}/samples/net/common/common.cmake)
