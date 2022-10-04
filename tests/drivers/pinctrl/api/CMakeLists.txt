# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(pinctrl_api)

zephyr_include_directories(src)
zephyr_include_directories(../common)

target_sources(app PRIVATE src/main.c src/pinctrl_test.c ../common/test_device.c)
