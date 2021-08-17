# Copyright (c) 2020 Intel Corporation.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(power_management_test)

target_sources(app PRIVATE src/dummy_driver.c)
if(${TEST_INTERFACES})
	target_sources(app PRIVATE src/pm_interfaces.c)
else()
	target_sources(app PRIVATE src/main.c)
endif()

target_include_directories(app PRIVATE
	${ZEPHYR_BASE}/kernel/include
	${ZEPHYR_BASE}/arch/${ARCH}/include
	${ZEPHYR_BASE}/subsys/pm)
