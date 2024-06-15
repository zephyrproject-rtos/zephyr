# Copyright (c) 2023 Intel Corporation.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lockfree_test)

target_sources(app PRIVATE src/test_spsc.c src/test_mpsc.c)

target_include_directories(app PRIVATE
  ${ZEPHYR_BASE}/include
	${ZEPHYR_BASE}/arch/${ARCH}/include)
