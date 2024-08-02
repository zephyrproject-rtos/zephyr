# Copyright 2024 Google LLC
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bbram)

target_sources(app
    PRIVATE
        include/fixture.h
        src/fixture.c
        src/read.c
        src/write.c
)
target_include_directories(app PRIVATE include)
