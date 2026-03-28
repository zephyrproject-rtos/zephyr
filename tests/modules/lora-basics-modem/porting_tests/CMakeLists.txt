# Copyright (c) 2025 Carlo Caione <ccaione@baylibre.com>
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(lbm_porting LANGUAGES C)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
