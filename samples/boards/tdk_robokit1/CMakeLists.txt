# Copyright (c) 2022 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tdk_robokit1)

target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/drivers/sensor/icm42688)
target_sources(app PRIVATE src/main.c)
