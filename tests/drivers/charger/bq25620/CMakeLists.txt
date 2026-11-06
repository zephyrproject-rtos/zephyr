# Copyright (c) 2026 Testo SE & Co. KGaA
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bq25620)

target_sources(app PRIVATE src/test_bq25620.c)
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/drivers/mfd)
