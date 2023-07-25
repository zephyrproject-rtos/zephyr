# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(cmsis_rtos_v1)

target_include_directories(app PRIVATE ${ZEPHYR_BASE}/include/zephyr/portability)
FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
