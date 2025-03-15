# Copyright (c) 2024 Basalte bv
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(sbom_spdx)
target_sources(app PRIVATE ${ZEPHYR_BASE}/misc/empty_file.c)

enable_testing()
include(CTest)

add_test(NAME spdx_gen COMMAND west spdx -d ${CMAKE_BINARY_DIR})
