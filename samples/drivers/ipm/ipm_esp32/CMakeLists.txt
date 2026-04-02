# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

set(REMOTE_ZEPHYR_DIR ${CMAKE_CURRENT_BINARY_DIR}/../ipm_esp32_remote/zephyr)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

message(STATUS "${BOARD} compile as Master in this sample")
project(ipm_esp32)

target_sources(app PRIVATE src/main.c src/procpu_shell.c)
