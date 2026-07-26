/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>
#include <zephyr/fs/fs.h>
#include "test_virtiofs.h"

#define MAX_PATH_LEN 256

int delete_dir_recursive(const char *path)
{
	struct fs_dir_t dir;
	struct fs_dirent entry;
	char full_path[MAX_PATH_LEN];

	fs_dir_t_init(&dir);

	int rc = fs_opendir(&dir, path);

	if (rc < 0) {
		return rc;
	}

	while (true) {
		rc = fs_readdir(&dir, &entry);
		if (rc < 0) {
			break;
		}

		if (entry.name[0] == 0) {
			/* End of directory */
			break;
		}

		int n = snprintf(full_path, sizeof(full_path), "%s/%s", path, entry.name);

		if (n < 0 || (size_t)n >= sizeof(full_path)) {
			rc = -ENAMETOOLONG;
			break;
		}

		if (entry.type == FS_DIR_ENTRY_DIR) {
			/* Recursively delete subdirectory contents first */
			rc = delete_dir_recursive(full_path);
			if (rc < 0) {
				break;
			}
		}

		rc = fs_unlink(full_path);
		if (rc < 0) {
			break;
		}
	}

	fs_closedir(&dir);

	return rc;
}
