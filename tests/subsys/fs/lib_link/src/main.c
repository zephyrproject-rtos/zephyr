/*
 * Copyright (c) 2025 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include <ff.h>
#include <lfs.h>

ZTEST_SUITE(lib_link, NULL, NULL, NULL, NULL, NULL);

ZTEST(lib_link, test_fs_libraries_linked)
{
	/*
	 * The primary assertion of this test is the build itself: with only
	 * CONFIG_FILE_SYSTEM_LIB_LINK enabled (no CONFIG_FILE_SYSTEM), the
	 * underlying file system libraries must compile and link. Referencing
	 * an API symbol from the libraries meant for direct application use
	 * additionally forces the linker to resolve them, so the libraries
	 * cannot be silently garbage-collected out of the image.
	 */
	zassert_not_null(f_mount, "FAT library not linked");
	zassert_not_null(lfs_mount, "littlefs library not linked");
}
