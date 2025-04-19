# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(sys_util_expr)

target_sources(app PRIVATE
	src/main.c
	src/logical.c
	src/shift.c
	src/compare.c
	src/add_sub.c
	src/multiply.c
	src/division.c
	src/decimal.c
)
