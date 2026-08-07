#
# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(rpmsgfs)

FILE(GLOB app_sources src/*.c)

target_include_directories(app PRIVATE ${LIBMETAL_INCLUDE_DIR} ${OPENAMP_INCLUDE_DIR})
target_compile_definitions(app PRIVATE
  BYPASS_FS_OPEN_FLAGS__CREATE_WITHOUT_RW_IS_NOT_ALLOWED
  BYPASS_FS_OPEN_FLAGS__APPEND_HAS_RW_BY_DEFAULT
  BYPASS_FS_UTIL__SIZE_IN_READDIR_NOT_AVAILABLE
)
target_sources(app PRIVATE
  ${app_sources}
  ../common/test_fs_util.c
  ../common/test_fs_open_flags.c
  ../common/test_fs_dirops.c
  ../common/test_fs_basic.c
  ../common/test_fs_mount_flags.c
  ../common/test_fs_mkfs.c
  ../common/test_fs_gc.c
)
