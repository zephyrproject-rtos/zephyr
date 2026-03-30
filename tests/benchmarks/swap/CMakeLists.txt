# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hello_world)

target_sources(app PRIVATE src/main.c)

# Needed to call z_swap()
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/kernel/include)
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/arch/${ARCH}/include)
