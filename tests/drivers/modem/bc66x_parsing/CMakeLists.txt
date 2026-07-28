# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bc66x_parsing)

file(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

target_include_directories(app PRIVATE ${ZEPHYR_BASE}/drivers/modem/vendor_standalone/quectel/bc66x)

target_sources(app PRIVATE ${ZEPHYR_BASE}/drivers/modem/vendor_standalone/quectel/bc66x/bc66x_utils.c)

target_include_directories(app PRIVATE ${ZEPHYR_BASE}/drivers/modem/)
