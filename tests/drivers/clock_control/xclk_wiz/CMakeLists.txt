# Copyright (C) 2026 Advanced Micro Devices, Inc. (AMD)
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(xlnx_clk_wiz_test)

file(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
