# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(fat_fs_api)

target_sources(app PRIVATE
		src/common.c
		src/main.c
		src/test_fat_fs.c
		src/test_fat_dir.c
		src/test_fat_mount.c
		src/test_fat_rename.c
		src/test_fat_rename.c
		src/test_fat_rd_only_mount.c
		src/test_fat_file.c
		../common/test_fs_open_flags.c)
target_sources_ifdef(CONFIG_FLASH app PRIVATE
		../common/test_fs_mkfs.c
		src/test_fat_mkfs.c)
