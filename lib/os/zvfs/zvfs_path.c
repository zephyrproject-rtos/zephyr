/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Path-based ZVFS operations over a mounted Zephyr file system.
 */

#include <errno.h>

#include <zephyr/fs/fs.h>
#include <zephyr/sys/zvfs_fs.h>

int zvfs_unlink(const char *path)
{
	int res = fs_unlink(path);

	if (res < 0) {
		errno = -res;
		return -1;
	}
	return 0;
}

int zvfs_rename(const char *old, const char *newp)
{
	int res = fs_rename(old, newp);

	if (res < 0) {
		errno = -res;
		return -1;
	}
	return 0;
}
