# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr COMPONENTS unittest REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(i2c_dw_target_con)

target_include_directories(testbinary PRIVATE ${ZEPHYR_BASE}/drivers/i2c)
target_sources(testbinary PRIVATE main.c)
