# Copyright (c) 2026 Dotcom IoT LLP
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ams_as7341_test)

file(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

zephyr_include_directories(${ZEPHYR_BASE}/drivers/sensor/ams/ams_as7341)
zephyr_include_directories(./src)
