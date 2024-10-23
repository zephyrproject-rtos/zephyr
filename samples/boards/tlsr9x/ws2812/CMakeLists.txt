# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ws2812)

target_sources(app PRIVATE src/main.c src/zephyr_ws2812.c)
