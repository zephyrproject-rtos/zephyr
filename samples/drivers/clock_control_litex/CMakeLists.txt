# Copyright (c) 2020 Antmicro <www.antmicro.com>
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(clock_control_litex)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
