# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

set(ZEPYHR_TOOLCHAIN_VARIANT "xt-clang")
set(XTENSA_CORE "nxp_rt600_RI23_11_newlib")
set(TOOLCHAIN_VER "RI-2023.11-linux")
set(XTENSA_TOOLCHAIN_PATH "/opt/xtensa/XtDevTools/install/tools/")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(remote)

target_sources(app PRIVATE src/main.c)

include("${CMAKE_CURRENT_LIST_DIR}/../../common/common.cmake")
