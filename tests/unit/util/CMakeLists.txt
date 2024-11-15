# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr COMPONENTS unittest REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(util)

target_sources(testbinary
  PRIVATE
  main.c
  ${ZEPHYR_BASE}/lib/utils/dec.c
  ${ZEPHYR_BASE}/lib/utils/utf8.c
)

if(CONFIG_CPP)
  # When testing for C++ force test file C++ compilation
  set_source_files_properties(main.c ${ZEPHYR_BASE}/lib/utils/dec.c PROPERTIES LANGUAGE CXX)
endif()
