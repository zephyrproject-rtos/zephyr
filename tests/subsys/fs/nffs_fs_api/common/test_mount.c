/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @filesystem
 * @brief test_filesystem
 * Demonstrates the ZEPHYR File System APIs
 */

#include <stdint.h>
#include <stdio.h>
#include <device.h>
#include <fs.h>
#include <ztest.h>
#include <ztest_assert.h>
#include "nffs_test_utils.h"

/* NFFS work area strcut */
static struct nffs_flash_desc flash_desc;

/* mounting info */
static struct fs_mount_t nffs_mnt = {
	.type = FS_NFFS,
	.mnt_point = "/nffs",
	.fs_data = &flash_desc,
};

static int test_mount(void)
{
	struct device *flash_dev;
	int res;

	flash_dev = device_get_binding(CONFIG_FS_NFFS_FLASH_DEV_NAME);
	if (!flash_dev) {
		return -ENODEV;
	}

	/* set backend storage dev */
	nffs_mnt.storage_dev = flash_dev;

	res = fs_mount(&nffs_mnt);
	if (res < 0) {
		TC_PRINT("Error mounting nffs [%d]\n", res);
		return TC_FAIL;
	}

	return TC_PASS;
}

void test_fs_mount(void)
{
	zassert_true(test_mount() == TC_PASS, NULL);
}
