# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(cmsis_rtos_v1_synchronization)

target_include_directories(app PRIVATE ${ZEPHYR_BASE}/include/cmsis_rtos_v1)
target_sources(app PRIVATE src/main.c)
