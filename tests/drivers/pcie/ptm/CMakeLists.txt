# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(pcie_ptm)

target_sources(app PRIVATE src/main.c)
target_compile_definitions(app PRIVATE CONFIG_PCIE_LOG_LEVEL=0)
