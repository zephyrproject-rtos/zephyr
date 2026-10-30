# Copyright (c) 2026 YunHung Hua
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(memc_userspace)

target_sources(app PRIVATE src/main.c)
