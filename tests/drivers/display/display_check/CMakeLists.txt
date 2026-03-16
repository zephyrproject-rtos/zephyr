# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(display)

# This project uses C source code from driver display sample
target_sources(app PRIVATE src/main.c ${ZEPHYR_BASE}/samples/drivers/display/src/display.c)
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/samples/drivers/display/src/)
