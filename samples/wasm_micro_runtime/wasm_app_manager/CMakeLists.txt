# Copyright (c) 2020 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.8.2)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(wasm_app_manager)

target_sources(app PRIVATE src/main.c)
