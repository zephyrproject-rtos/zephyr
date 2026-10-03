# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-FileCopyrightText: Copyright (c) 2026 Dev It Wise
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(dns_fuzz)

target_sources(app PRIVATE src/main.c)

# UBSan reports and continues by default, so an input that triggers
# undefined behaviour would be recorded by libFuzzer as a clean run.
zephyr_compile_options(-fno-sanitize-recover=all)
