/*
 * Copyright (c) 2026 Rishav Chakraborty.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/fs_utils.h>

#include <zephyr/logging/log.h>

#include "fs_internal.h"

LOG_MODULE_DECLARE(fs, CONFIG_FS_LOG_LEVEL);

/* Create each component of buf from start onwards; buf holds the full path. */
static int mkdir_p_buf(char *buf, char *start)
{
	int rc;

	for (char *p = start;; p++) {
		if (*p != '/' && *p != '\0') {
			continue;
		}

		char saved = *p;

		if (p[-1] != '/') {
			*p = '\0';
			struct fs_dirent ent;

			rc = fs_stat(buf, &ent);
			if (rc == 0) {
				if (ent.type != FS_DIR_ENTRY_DIR) {
					return -ENOTDIR;
				}
			} else if (rc == -ENOENT) {
				rc = fs_mkdir(buf);
				if (rc < 0 && rc != -EEXIST) {
					return rc;
				}
			} else {
				return rc;
			}

			*p = saved;
		}

		if (*p == '\0') {
			break;
		}
	}

	return 0;
}

int fs_mkdir_p(const char *abs_path)
{
	if ((abs_path == NULL) || (abs_path[0] != '/')) {
		LOG_ERR("invalid directory name!!");
		return -EINVAL;
	}

	if (abs_path[1] == '\0') {
		/* The VFS root always exists. */
		return 0;
	}

	size_t len = strlen(abs_path);

	if (len >= CONFIG_FILE_SYSTEM_UTILS_MAX_PATH_LEN) {
		return -ENAMETOOLONG;
	}

	char buf[CONFIG_FILE_SYSTEM_UTILS_MAX_PATH_LEN];
	struct fs_mount_t *mp;
	int rc;

	rc = fs_get_mnt_point(&mp, abs_path, NULL);
	if (rc < 0) {
		LOG_ERR("mount point not found!!");
		return rc;
	}

	memcpy(buf, abs_path, len + 1);
	char *start = buf + mp->mountp_len;

	if (*start == '/') {
		start++;
	}

	return mkdir_p_buf(buf, start);
}
