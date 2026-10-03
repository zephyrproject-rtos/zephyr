# Copyright (c) 2026 Fabien Proriol
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr COMPONENTS unittest REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(knx_l7_apdu)

target_sources(testbinary PRIVATE src/main.c)
target_include_directories(testbinary PRIVATE
    ${ZEPHYR_BASE}/subsys/knx
)
target_compile_options(testbinary PRIVATE -Wno-unused-function)
