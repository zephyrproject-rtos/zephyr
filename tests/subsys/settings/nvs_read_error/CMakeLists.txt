# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS "$ENV{ZEPHYR_BASE}")
project(settings_nvs_read_error)

target_sources(app PRIVATE src/main.c)
target_link_options(app PUBLIC
  "-Wl,--wrap=nvs_read"
  "-Wl,--wrap=nvs_write"
  "-Wl,--wrap=nvs_delete"
)
