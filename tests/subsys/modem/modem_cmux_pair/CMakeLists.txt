# Copyright (c) 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(modem_cmux_pair_test)

target_sources(app PRIVATE src/main.c ../mock/modem_backend_mock.c)
target_include_directories(app PRIVATE ../mock)
