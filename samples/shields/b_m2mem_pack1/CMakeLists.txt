# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS ${ZEPHYR_BASE})
project(b_m2mem_pack1)

target_sources(app PRIVATE src/main.c)
