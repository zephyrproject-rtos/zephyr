# Copyright (c) 2020, George Gkinis
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})
project(hx711)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
