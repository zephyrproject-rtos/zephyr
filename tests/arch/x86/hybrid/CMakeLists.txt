# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2023 Intel Corp.

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(intel_hybrid)

target_sources(app PRIVATE src/smpboot.c)
