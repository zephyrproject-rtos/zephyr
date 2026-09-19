# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-FileCopyrightText: Copyright 2026 Mohammed El Amrani
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(log_gc)

target_sources(app PRIVATE
  src/main.c
  src/used_module.c
  src/unused_module.c
)
