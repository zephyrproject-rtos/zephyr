# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(armlink_scatter)

include(${CMAKE_CURRENT_LIST_DIR}/armlink_scatter.cmake)

target_sources(app PRIVATE ${ZEPHYR_BASE}/misc/empty_file.c)
