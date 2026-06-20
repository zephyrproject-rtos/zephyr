/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>

#include <zephyr/sys/fdtable.h>
#include <zephyr/sys/internal/fdtable_priv.h>
#include <zephyr/sys/zvfs_libc.h>

int zvfs_lock_file(FILE *file, k_timeout_t timeout)
{
	int fd;
	int prev_errno;
	struct fd_entry *entry;

	fd = zvfs_libc_fileno(file);
	prev_errno = errno;
	if (fd < 0) {
		if (errno != prev_errno) {
			errno = prev_errno;
		}
		return -1;
	}

	entry = zvfs_fd_entry_get(fd);
	if (entry == NULL) {
		if (errno != prev_errno) {
			errno = prev_errno;
		}
		return -1;
	}

	return k_mutex_lock(&entry->lock, timeout);
}

int zvfs_unlock_file(FILE *file)
{
	int fd;
	int prev_errno;
	struct fd_entry *entry;

	fd = zvfs_libc_fileno(file);
	prev_errno = errno;
	if (fd < 0) {
		if (errno != prev_errno) {
			errno = prev_errno;
		}
		return -1;
	}

	entry = zvfs_fd_entry_get(fd);
	if (entry == NULL) {
		if (errno != prev_errno) {
			errno = prev_errno;
		}
		return -1;
	}

	return k_mutex_unlock(&entry->lock);
}

int zvfs_getc_unlocked(FILE *stream)
{
	int fd;
	int res;
	char buf;

	fd = zvfs_libc_fileno(stream);
	if (fd < 0) {
		return EOF;
	}

	res = zvfs_rw_unlocked(fd, &buf, 1, false, NULL);
	if (res <= 0) {
		return EOF;
	}

	return (int)buf;
}

int zvfs_putc_unlocked(int c, FILE *stream)
{
	int fd;
	int res;
	char buf = (char)c;

	fd = zvfs_libc_fileno(stream);
	if (fd < 0) {
		return EOF;
	}

	res = zvfs_rw_unlocked(fd, &buf, 1, true, NULL);
	if (res <= 0) {
		return EOF;
	}

	return c;
}
