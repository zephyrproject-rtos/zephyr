# Copyright (c) 2026 Intel Corporation.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(word_granular_access)

file(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
