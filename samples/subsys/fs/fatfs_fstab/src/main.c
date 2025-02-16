/*
 * Copyright (c) 2025 Endress+Hauser AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ff.h>
#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/disk_access.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

int main(void)
{
	struct fs_file_t file;
	int rc;
	static const char data[] = "Hello";

	fs_file_t_init(&file);
	rc = fs_open(&file, "/RAM:/hello.txt", FS_O_CREATE | FS_O_WRITE);
	if (rc != 0) {
		LOG_ERR("Accessing filesystem failed");
		return rc;
	}
	rc = fs_write(&file, data, strlen(data));
	if (rc != strlen(data)) {
		LOG_ERR("Writing filesystem failed");
		return rc;
	}
	rc = fs_close(&file);
	if (rc != 0) {
		LOG_ERR("Closing file failed");
		return rc;
	}

	LOG_INF("Filesystem access successful");
	return 0;
}
