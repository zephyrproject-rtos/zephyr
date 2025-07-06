/*
 * Copyright (c) 2024 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fs_priv.h"

#include <errno.h>
#include <string.h>

#include <zephyr/fs/fs.h>
#include <zephyr/posix/dirent.h>
#include <zephyr/sys/util.h>

int readdir_r(DIR *dirp, struct dirent *entry, struct dirent **result)
{
	int rc;
	struct fs_dirent de;
	struct posix_fs_desc *const ptr = dirp;

	if (result == NULL) {
		return EINVAL;
	}

	if (entry == NULL) {
		*result = NULL;
		return EINVAL;
	}

	if (dirp == NULL) {
		*result = NULL;
		return EBADF;
	}

	rc = fs_readdir(&ptr->dir, &de);
	if (rc < 0) {
		*result = NULL;
		return -rc;
	}

	strncpy(entry->d_name, de.name, MIN(sizeof(entry->d_name), sizeof(de.name)));
	entry->d_name[sizeof(entry->d_name) - 1] = '\0';

	if (entry->d_name[0] == '\0') {
		*result = NULL;
		return 0;
	}

	*result = entry;
	return 0;
}
