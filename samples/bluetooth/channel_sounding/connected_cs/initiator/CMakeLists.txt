# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(connected_cs_initiator)

target_sources(app PRIVATE ../../src/connected_cs_initiator.c ../../src/distance_estimation.c)

zephyr_library_include_directories(
        ${CMAKE_CURRENT_LIST_DIR}/../../include
)
