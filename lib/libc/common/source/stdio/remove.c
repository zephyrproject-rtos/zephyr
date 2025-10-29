/*
 * Copyright (c) 2024 Linumiz.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/sys/util_macro.h>

/**
 *
 * @brief deletes a name from the filesystem
 *
 * @return On success, zero is returned. On error, -1 is returned
 * and errno is set to indicate the error.
 */

int remove(const char *path)
{
	if (!IS_ENABLED(CONFIG_FILE_SYSTEM)) {
		errno = ENOTSUP;
		return -1;
	}

	return zvfs_unlink(path);
}
