/*
 * Copyright (c) 2025 Endress+Hauser AG
 * Copyright (c) 2025 Arch-Embedded B.V.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/disk_access.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define AUTOMOUNT_NODE DT_NODELABEL(ext2fs1)
#define MOUNT_POINT DT_PROP(AUTOMOUNT_NODE, mount_point)

FS_FSTAB_DECLARE_ENTRY(AUTOMOUNT_NODE);

#define FILE_PATH MOUNT_POINT "/hello.txt"

int main(void)
{
	struct fs_file_t file;
	int rc;
	static const char data[] = "Hello";
	/* You can get direct mount point to automounted node too */
	struct fs_mount_t *auto_mount_point = &FS_FSTAB_ENTRY(AUTOMOUNT_NODE);
	struct fs_dirent stat;

	fs_file_t_init(&file);

	rc = fs_open(&file, FILE_PATH, FS_O_CREATE | FS_O_WRITE);
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

	/* You can unmount the automount node */
	rc = fs_unmount(auto_mount_point);
	if (rc != 0) {
		LOG_ERR("Failed to do unmount");
		return rc;
	};

	/* And mount it back */
	rc = fs_mount(auto_mount_point);
	if (rc != 0) {
		LOG_ERR("Failed to remount the auto-mount node");
		return rc;
	}

	/* Is the file still there? */
	rc = fs_stat(FILE_PATH, &stat);
	if (rc != 0) {
		LOG_ERR("File status check failed %d", rc);
		return rc;
	}

	LOG_INF("Filesystem access successful");

	return 0;
}
