# Copyright (c) 2023 North River Systems Ltd
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(device)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

# Include the INA23x driver path and unit-test path for private header inclusion
zephyr_include_directories(./src)
zephyr_include_directories(${ZEPHYR_BASE}/drivers/sensor/ti/ina23x)
