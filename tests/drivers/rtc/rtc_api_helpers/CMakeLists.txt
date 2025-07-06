# Copyright (c) 2022 Bjarki Arge Andreasen
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(rtc_api)

target_sources(app PRIVATE
	src/main.c
	src/test_rtc_time_to_tm.c
	src/test_rtc_calibration_from_frequency.c
)
