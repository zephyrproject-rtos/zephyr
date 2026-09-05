# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-FileCopyrightText: Copyright (c) 2026 Dev It Wise
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_settings_nvs_data_crc)

zephyr_include_directories(${ZEPHYR_BASE}/subsys/settings/include)

target_sources(app PRIVATE src/main.c)
