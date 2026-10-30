/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/storage/disk_access.h>
#include <ff.h>

#define RAM_DISK "RAM:"

static FATFS fat_fs;
static struct fs_mount_t mp = {
	.type = FS_FATFS,
	.mnt_point = "/RAM:",
	.fs_data = &fat_fs,
};

int main(void)
{
	int ret;

	printk("fs_unmount_path sample\n");

	ret = fs_unmount_path(NULL);
	if (ret != -EINVAL) {
		printk("fs_unmount_path(NULL) expected -EINVAL got %d\n", ret);
		return 1;
	}

	ret = fs_mkfs(FS_FATFS, (uintptr_t)RAM_DISK, NULL, 0);
	if (ret != 0) {
		printk("fs_mkfs failed: %d\n", ret);
		return 1;
	}

	ret = fs_mount(&mp);
	if (ret != 0) {
		printk("fs_mount failed: %d\n", ret);
		return 1;
	}

	ret = fs_unmount_path("/RAM:");
	if (ret != 0) {
		printk("fs_unmount_path failed: %d\n", ret);
		return 1;
	}

	printk("fs_unmount_path OK\n");

	return 0;
}
