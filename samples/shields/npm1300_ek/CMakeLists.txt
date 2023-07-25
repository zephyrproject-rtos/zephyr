# Copyright (c) 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

set(SHIELD npm1300_ek)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(npm1300_ek)
target_sources(app PRIVATE src/main.c)
