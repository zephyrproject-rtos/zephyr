# SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(pmbus_core)

target_sources(app PRIVATE src/main.c ../common/fake_pmbus_dev.c)
target_include_directories(app PRIVATE ../common)
