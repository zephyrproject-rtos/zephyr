# Copyright (c) 2020 PHYTEC Messtechnik GmbH
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_modbus)

target_sources(app PRIVATE
    src/main.c
    src/test_modbus_client.c
    src/test_modbus_server.c
    src/test_modbus_raw.c
    )
