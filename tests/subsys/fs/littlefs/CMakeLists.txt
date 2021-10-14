# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(littlefs)

FILE(GLOB app_sources src/*.c)
# LittleFS opens files for RW by default if no flags given and crashes,
# instead of returning error when attempting to perform operation that
# a file has not been opened for.
target_compile_definitions(app PRIVATE
		BYPASS_FS_OPEN_FLAGS_LFS_ASSERT_CRASH
		BYPASS_FS_OPEN_FLAGS_LFS_RW_IS_DEFAULT
)
target_sources(app PRIVATE ${app_sources} ../common/test_fs_open_flags.c)
