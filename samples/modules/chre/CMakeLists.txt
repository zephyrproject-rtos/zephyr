# Copyright (c) Google LLC
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(chre_sample)

FILE(GLOB app_sources src/*.cpp)
target_sources(app PRIVATE ${app_sources})
target_include_directories(app PRIVATE include)
