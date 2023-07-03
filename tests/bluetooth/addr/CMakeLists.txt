# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(app)

target_sources(app PRIVATE
	src/test_bt_addr_le_eq.c
	src/test_bt_addr_from_str.c
)
