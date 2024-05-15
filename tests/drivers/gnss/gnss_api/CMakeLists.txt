# Copyright (c) 2024 Trackunit Corporation
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(gnss_api)

target_sources(app PRIVATE
	src/test_enabled_systems.c
	src/test_fix_rate.c
	src/test_navigation_mode.c
	src/test_suite.c
)
