# Copyright (c) 2026 Rishav Chakraborty
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(arc_nocache)

target_sources(app PRIVATE src/main.c)
