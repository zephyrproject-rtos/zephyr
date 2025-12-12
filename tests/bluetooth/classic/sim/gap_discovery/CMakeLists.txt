# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
set(NO_QEMU_SERIAL_BT_SERVER 1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bluetooth)

if(CONFIG_TEST_GAP_PERIPHERAL)
FILE(GLOB app_sources src/test_peripheral.c)
endif ()

if(CONFIG_TEST_GAP_CENTRAL)
FILE(GLOB app_sources src/test_central.c)
endif()

target_sources(app PRIVATE ${app_sources})
