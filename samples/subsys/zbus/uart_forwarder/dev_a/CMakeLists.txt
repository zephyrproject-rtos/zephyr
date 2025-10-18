#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(zbus_uart_forwarder_dev_a)


zephyr_library_include_directories(
  src
  ../common
)

# Define which device this is
zephyr_compile_definitions(ZBUS_DEVICE_A=1)

target_sources(app PRIVATE src/main.c)
