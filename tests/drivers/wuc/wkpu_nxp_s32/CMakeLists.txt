# SPDX-FileCopyrightText: Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(wkpu_nxp_s32)

set(WKPU_TEST_SOURCES 64 CACHE STRING "Number of WKPU sources")

target_compile_definitions(app PRIVATE
  CONFIG_NXP_S32_WKPU_SOURCES_MAX=${WKPU_TEST_SOURCES}
)
target_sources(app PRIVATE
  src/main.c
)
