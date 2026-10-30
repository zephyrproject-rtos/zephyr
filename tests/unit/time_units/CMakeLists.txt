# Copyright 2022 Meta
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr COMPONENTS unittest REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(time_units)

FILE(GLOB app_sources *.c)
target_sources(testbinary PRIVATE ${app_sources})
target_sources(testbinary PRIVATE ${ZEPHYR_BASE}/lib/os/div64.c)
# The reciprocal is only derived when the optimizer folds it.
set_source_files_properties(reciprocal.c PROPERTIES COMPILE_OPTIONS -Os)
