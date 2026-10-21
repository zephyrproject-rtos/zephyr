# SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr COMPONENTS unittest REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(bitmap)

target_sources(testbinary PRIVATE
  main.c
  ${ZEPHYR_BASE}/lib/utils/bitmap.c
)

if(CONFIG_CPP)
  set_source_files_properties(main.c ${ZEPHYR_BASE}/lib/utils/bitmap.c PROPERTIES LANGUAGE CXX)
endif()
