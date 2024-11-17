# Copyright (c) 2024 Arif Balik <arifbalik@outlook.com>
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(stm32_tsc)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
