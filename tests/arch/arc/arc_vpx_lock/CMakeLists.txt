# Copyright (c) 2024 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(arc_vpx_lock)

target_sources(app PRIVATE
    src/main.c
    )

if(COMPILER STREQUAL arcmwdt)
zephyr_include_directories(${ARCMWDT_TOOLCHAIN_PATH}/MetaWare/arc/lib/src/fx/include/)
endif()
