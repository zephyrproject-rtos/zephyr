# Copyright (c) 2024 Google LLC
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(i2c_emul)

target_sources(app PRIVATE
    src/emulated_target.cpp
    src/test_forwarding.cpp
)
target_include_directories(app PRIVATE include)
