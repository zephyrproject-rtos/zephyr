/*
 * Copyright (c) 2025 Golioth, Inc.
 * Copyright (c) 2025 Marcin Niestroj
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/fs/fs.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mount);

#include "mount.h"

struct fs_mount_t test_mp = {
	.type = FS_NATIVE_MOUNT,
	.mnt_point = "/test_fs",
	.fs_data = TEST_FS_DIR,
	.storage_dev = NULL,
};

static int fs_rm_child(const char *path, struct fs_dir_t *dp)
{
	struct fs_dirent entry;
	char *path_child;
	size_t path_child_len;
	int err;

	err = fs_readdir(dp, &entry);
	if (err) {
		LOG_ERR("Failed to readdir during rm: %d", err);
		return err;
	}

	if (entry.name[0] == '\0') {
		return -EAGAIN;
	}

	path_child_len = strlen(path) + strlen(entry.name) + sizeof("/");

	path_child = malloc(path_child_len);
	if (!path_child) {
		return -ENOMEM;
	}

	snprintf(path_child, path_child_len, "%s/%s", path, entry.name);

	if (entry.type == FS_DIR_ENTRY_DIR) {
		err = fs_rm_all(path_child);
		if (err) {
			goto free_child_path;
		}
	}

	LOG_INF("Unlinking %s", path_child);

	err = fs_unlink(path_child);

free_child_path:
	free(path_child);

	return err;
}

int fs_rm_all(const char *path)
{
	struct fs_dir_t dp = {};
	int err = 0;

	LOG_INF("rm all %s", path);

	err = fs_opendir(&dp, path);
	if (err) {
		LOG_ERR("opendir failed: %d", err);
		return err;
	}

	while (err == 0) {
		err = fs_rm_child(path, &dp);
	}

	return fs_closedir(&dp);
}
