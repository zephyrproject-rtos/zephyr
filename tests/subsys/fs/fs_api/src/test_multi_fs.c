/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_fs.h"
/* amount of file system */
#define NUM_FS 2
#define TEST_FS_NAND1 "/NAND:"
#define TEST_FS_NAND2 "/MMCBLOCK:"

static struct test_fs_data test_data;

static struct fs_mount_t test_fs_mnt_1 = {
		.type = TEST_FS_1,
		.mnt_point = TEST_FS_NAND1,
		.fs_data = &test_data,
};

static struct fs_mount_t test_fs_mnt_2 = {
		.type = TEST_FS_2,
		.mnt_point = TEST_FS_NAND2,
		.fs_data = &test_data,
};

static int test_fs_init(void)
{
	if (fs_register(TEST_FS_1, &temp_fs)) {
		return -EINVAL;
	}

	if (fs_mount(&test_fs_mnt_1)) {
		return -EINVAL;
	}

	if (fs_register(TEST_FS_2, &temp_fs)) {
		return -EINVAL;
	}

	if (fs_mount(&test_fs_mnt_2)) {
		return -EINVAL;
	}

	return 0;
}

static int test_fs_readmount(void)
{
	int ret;
	int mnt_nbr = 0;
	const char *mnt_name;

	do {
		ret = fs_readmount(&mnt_nbr, &mnt_name);
		if (ret < 0) {
			break;
		}


	} while (true);

	if (mnt_nbr == NUM_FS) {
		return 0;
	}

	return TC_FAIL;
}

static int test_fs_deinit(void)
{
	if (fs_unregister(TEST_FS_1, &temp_fs)) {
		return -EINVAL;
	}

	if (fs_unmount(&test_fs_mnt_1)) {
		return -EINVAL;
	}

	if (fs_unregister(TEST_FS_2, &temp_fs)) {
		return -EINVAL;
	}

	if (fs_unmount(&test_fs_mnt_2)) {
		return -EINVAL;
	}
	return 0;
}

static int test_fs_unsupported(void)
{
	if (fs_register(FS_TYPE_END, &temp_fs) == 0) {
		return TC_FAIL;
	}

	if (fs_unregister(FS_TYPE_END, &temp_fs) == 0) {
		return TC_FAIL;
	}

	return 0;
}

/**
 * @brief Multi file systems register and unregister
 *
 * @details register and unregister two file systems to test
 *          the system support multiple file system simultanously
 *
 * @addtogroup filesystem_api
 *
 * @{
 */

void test_fs_register(void)
{
	zassert_true(test_fs_init() == 0, "Failed to register filesystems");
	zassert_true(test_fs_readmount() == 0, "Failed to readmount");
	zassert_true(test_fs_deinit() == 0, "Failed to unregister filesystems");
	zassert_true(test_fs_unsupported() == 0, "Supported other file system");
}

/**
 * @}
 */
