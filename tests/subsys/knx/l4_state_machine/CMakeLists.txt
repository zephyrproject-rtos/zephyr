# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Fabien Proriol

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr COMPONENTS unittest REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(knx_l4_state_machine)

target_sources(testbinary PRIVATE src/main.c)
target_include_directories(testbinary PRIVATE
    ${ZEPHYR_BASE}/subsys/knx
)

# Stub-heavy test file; suppress unused-function warnings from stubs
target_compile_options(testbinary PRIVATE -Wno-unused-function)
