# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})
project(philosophers_cmsis_rtos_v1)

target_include_directories(app PRIVATE ${ZEPHYR_BASE}/include/cmsis_rtos_v1)
target_sources(app PRIVATE src/main.c)
