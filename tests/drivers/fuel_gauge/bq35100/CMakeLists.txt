# Copyright (c) 2025 Orgatex GmbH
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(device)

file(GLOB app_sources src/test_bq35100.c)
target_sources(app PRIVATE ${app_sources})
