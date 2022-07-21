# Copyright (c) 2022 Würth Elektronik eiSos GmbH & Co. KG
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(wsen_hids)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
