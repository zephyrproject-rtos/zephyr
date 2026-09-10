# Copyright (c) 2025 Aerlync Labs Inc.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(min_heap_ref)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
