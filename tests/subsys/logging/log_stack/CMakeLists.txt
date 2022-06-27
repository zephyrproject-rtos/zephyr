# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(log_stack)

target_sources(app PRIVATE src/main.c)
if(CONFIG_CPLUSPLUS)
  # When testing for C++ force test file C++ compilation
  set_source_files_properties(src/main.c PROPERTIES LANGUAGE CXX)
endif()
