/*
 * Copyright (c) 2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 * Copyright (c) 2025 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fs_shell, CONFIG_LOG_DEFAULT_LEVEL);

FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(storage);

static struct fs_mount_t lfs_storage_mnt = {
	.type = FS_LITTLEFS,
	.fs_data = &storage,
	.storage_dev = (void *)FIXED_PARTITION_ID(storage_partition),
	.mnt_point = "/lfs",
};

int main(void)
{
	int rc;
	const char *file_path = "/lfs/test.txt";
	const char *file_content = "Hello from Zephyr!\n";

	/* Mount the filesystem */
	rc = fs_mount(&lfs_storage_mnt);
	if (rc >= 0) {
		LOG_INF("Mounted at %s", lfs_storage_mnt.mnt_point);
	} else {
		LOG_ERR("Mount failed: %d", rc);
	}

	/* Create a test.txt file */
	struct fs_file_t file;

	fs_file_t_init(&file);

	rc = fs_open(&file, file_path, FS_O_CREATE | FS_O_WRITE);
	if (rc >= 0) {
		fs_write(&file, file_content, strlen(file_content));
		fs_close(&file);
		LOG_INF("Created test file: %s", file_path);
	} else {
		LOG_ERR("Failed to create test file: %s", file_path);
	}

	LOG_INF("Try: \"fs cat /lfs/test.txt\"");

	return 0;
}
