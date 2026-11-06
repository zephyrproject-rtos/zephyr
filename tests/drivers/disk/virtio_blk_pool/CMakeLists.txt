# SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(virtio_blk_pool)

target_include_directories(app PRIVATE "${ZEPHYR_BASE}/drivers/disk")
target_sources(app PRIVATE src/main.c)
