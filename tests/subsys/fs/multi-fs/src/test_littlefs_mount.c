/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <ztest.h>
#include <zephyr/fs/littlefs.h>
#include "test_fs_shell.h"

#if !defined(CONFIG_FILE_SYSTEM_SHELL)
FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(storage);

struct fs_mount_t littlefs_mnt = {
	.type = FS_LITTLEFS,
	.fs_data = &storage,
	.storage_dev = (void *)FLASH_AREA_ID(storage),
	.mnt_point = "/littlefs"
};

static int test_mount(void)
{
	int res;

	res = fs_mount(&littlefs_mnt);
	if (res < 0) {
		TC_PRINT("Error mounting littlefs [%d]\n", res);
		return TC_FAIL;
	}

	return TC_PASS;
}
#endif

void test_littlefs_mount(void)
{
#ifdef CONFIG_FILE_SYSTEM_SHELL
	test_fs_littlefs_mount();
#else
	zassert_true(test_mount() == TC_PASS, NULL);
#endif
}
