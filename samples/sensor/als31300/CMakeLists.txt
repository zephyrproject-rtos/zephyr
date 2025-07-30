# Copyright (c) 2025 Croxel Inc.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(als31300)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
