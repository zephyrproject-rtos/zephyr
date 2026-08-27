/*
 * Copyright (c) 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/fs/fs.h>

int delete_dir_recursive(const char *path)
{
	struct fs_dir_t dir;
	struct fs_dirent entry;
	char full_path[MAX_FILE_NAME + 2];

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

		snprintf(full_path, sizeof(full_path), "%s/%s", path, entry.name);

		if (entry.type == FS_DIR_ENTRY_DIR) {
			/* Recursively delete subdirectory */
			rc = delete_dir_recursive(full_path);

			if (rc < 0) {
				break;
			}
		}

		/* Delete file */
		rc = fs_unlink(full_path);

		if (rc < 0) {
			break;
		}
	}

	fs_closedir(&dir);

	return rc;
}
