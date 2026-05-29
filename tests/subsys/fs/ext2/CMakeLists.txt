# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ext2tests)

FILE(GLOB app_sources src/*.c)
FILE(GLOB common_sources
  ../common/test_fs_util.c
  ../common/test_fs_mkfs.c
  ../common/test_fs_basic.c
  ../common/test_fs_dirops.c
  ../common/test_fs_open_flags.c
  ../common/test_fs_mount_flags.c
)
target_sources(app PRIVATE
  ${app_sources}
  ${common_sources}
)
