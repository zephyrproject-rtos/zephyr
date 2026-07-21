# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(pkt_filter)

target_sources(app PRIVATE src/main.c)

include(${ZEPHYR_BASE}/samples/net/common/common.cmake)
