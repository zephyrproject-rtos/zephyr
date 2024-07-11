# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(llext_edk_test LANGUAGES C)

target_sources(app PRIVATE src/main.c)
zephyr_include_directories(include)
zephyr_include_directories(${ZEPHYR_BASE}/boards/native/common)
