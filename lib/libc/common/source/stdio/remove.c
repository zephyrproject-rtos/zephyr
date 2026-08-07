/*
 * Copyright (c) 2024 Linumiz.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>
#include <zephyr/fs/fs.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys/zvfs_fs.h>

/**
 *
 * @brief deletes a name from the filesystem
 *
 * @return On success, zero is returned. On error, -1 is returned
 * and errno is set to indicate the error.
 */

int remove(const char *path)
{
	if (IS_ENABLED(CONFIG_ZVFS_FS)) {
		/* zvfs_remove() handles files and empty directories, as ISO C requires */
		return zvfs_remove(path);
	}

	if (IS_ENABLED(CONFIG_FILE_SYSTEM)) {
		int ret = fs_unlink(path);

		if (ret < 0) {
			errno = -ret;
			return -1;
		}

		return 0;
	}

	errno = ENOTSUP;
	return -1;
}
