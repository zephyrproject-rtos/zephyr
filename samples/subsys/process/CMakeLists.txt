# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(subprocess_sample)

target_sources(app PRIVATE
  src/builtin.c
  src/ext.c
  src/llext.c
  src/main.c
)

set(ext_name ext)
set(ext_src src/${ext_name}.c)
set(ext_bin ${PROJECT_BINARY_DIR}/llext/${ext_name}.llext)
set(ext_inc ${ZEPHYR_BINARY_DIR}/include/generated/${ext_name}_ext.inc)
add_llext_target(
  ${ext_name}
  OUTPUT  ${ext_bin}
  SOURCES ${ext_src}
)
generate_inc_file_for_target(app ${ext_bin} ${ext_inc})
