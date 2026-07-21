# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

message(STATUS "${BOARD} compiles as remote in this sample")
project(ipm_esp32_remote)

target_sources(app PRIVATE src/main.c)
