# Copyright 2023 Meta
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr COMPONENTS unittest REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(pot_test)

FILE(GLOB app_sources *.c *.cpp)
target_sources(testbinary PRIVATE ${app_sources})

# gcc
target_compile_options(testbinary PRIVATE -Wno-overflow)

# clang
target_compile_options(testbinary PRIVATE -Wno-shift-count-overflow -Wno-integer-overflow)
