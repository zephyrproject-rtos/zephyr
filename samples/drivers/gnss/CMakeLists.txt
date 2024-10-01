# Copyright (c) 2023 Trackunit Corporation
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(gnss_sample)

target_sources(app PRIVATE src/main.c)
