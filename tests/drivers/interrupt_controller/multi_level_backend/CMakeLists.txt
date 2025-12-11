# Copyright (c) 2024 Meta
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(intc_multi_level_backend)

# required for "sw_isr_common.h"
zephyr_library_include_directories(${ZEPHYR_BASE}/arch/common/include)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
