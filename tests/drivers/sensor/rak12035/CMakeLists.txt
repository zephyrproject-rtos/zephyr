# Copyright (c) 2026 RAKwireless Technology Limited
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(rak12035)

target_sources(app PRIVATE src/rak12035_test.c)
zephyr_include_directories(${ZEPHYR_BASE}/drivers/sensor/rakwireless/rak12035)
