# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(stm32_ptp_output_timing)

target_include_directories(app PRIVATE ${ZEPHYR_BASE}/drivers/ethernet)
target_sources(app PRIVATE src/main.c)
