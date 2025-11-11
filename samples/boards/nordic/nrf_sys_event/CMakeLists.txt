# Copyright 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(soc_sys_event)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
