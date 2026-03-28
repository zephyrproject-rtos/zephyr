 # SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya
 # SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tmc50xx)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
