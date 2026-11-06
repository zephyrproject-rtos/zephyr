# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(zfence)

# Common tests compiled for every platform.
target_sources(app PRIVATE
  zfence_test.c
)

# Platform-specific overflow and boundary tests.
if(CONFIG_64BIT)
  target_sources(app PRIVATE zfence_overflow_64bit_test.c)
else()
  target_sources(app PRIVATE zfence_overflow_32bit_test.c)
endif()
