# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS "$ENV{ZEPHYR_BASE}")
project(settings_load_error)

target_sources(app PRIVATE src/main.c)
target_include_directories(app PRIVATE "${ZEPHYR_BASE}/subsys/settings/src")
