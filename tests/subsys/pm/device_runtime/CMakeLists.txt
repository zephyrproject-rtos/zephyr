# Copyright (c) 2021 Intel Corporation.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(device_runtime_test)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
