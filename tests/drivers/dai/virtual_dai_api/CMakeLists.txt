# Copyright (c) 2025 Suraj Sonawane <surajsonawane0215@gmail.com>
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(virtual_dai_api)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
