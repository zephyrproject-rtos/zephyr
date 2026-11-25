# Copyright (c) 2026 BeagleBoard.org Foundation
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ti_k3_esm)

target_sources(app PRIVATE src/main.c)
