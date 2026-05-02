# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(arm_switch)

target_sources(app PRIVATE src/main.c)

# Need to unit test arch headers
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/arch/${ARCH}/include)
