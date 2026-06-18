# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(native_mount)

file(GLOB app_sources src/*.c)

target_sources(app PRIVATE
  ${app_sources}
  ../common/test_fs_util.c
  ../common/test_fs_open_flags.c
  ../common/test_fs_dirops.c
  ../common/test_fs_basic.c
)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/test_fs)

target_compile_definitions(app PRIVATE
  TEST_FS_DIR="${CMAKE_CURRENT_BINARY_DIR}/test_fs"
)
