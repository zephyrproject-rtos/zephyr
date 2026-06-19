/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>

#include <zephyr/sys/fdtable.h>
#include <zephyr/sys/zvfs.h>
#include <zephyr/sys/zvfs_libc.h>
#include <zephyr/sys/zvfs_fs.h>

#define FOPEN_CREATE_MODE 0644

#define ZVFS_O_ACCMODE (ZVFS_O_RDONLY | ZVFS_O_WRONLY | ZVFS_O_RDWR)

static int zvfs_fopen_mode_to_flags(const char *mode, int *flags)
{
	if (mode == NULL) {
		return -1;
	}

	switch (mode[0]) {
	case 'r':
		*flags = ZVFS_O_RDONLY;
		break;
	case 'w':
		*flags = ZVFS_O_WRONLY | ZVFS_O_CREAT | ZVFS_O_TRUNC;
		break;
	case 'a':
		*flags = ZVFS_O_WRONLY | ZVFS_O_CREAT | ZVFS_O_APPEND;
		break;
	default:
		return -1;
	}

	for (const char *m = mode + 1; *m != '\0'; m++) {
		switch (*m) {
		case '+':
			*flags = (*flags & ~ZVFS_O_ACCMODE) | ZVFS_O_RDWR;
			break;
		case 'x':
			*flags |= ZVFS_O_EXCL;
			break;
		case 'b':
		case 't':
			break;
		default:
			break;
		}
	}

	return 0;
}

FILE *fopen(const char *name, const char *mode)
{
	int flags;
	int fd;
	FILE *fp;

	if (zvfs_fopen_mode_to_flags(mode, &flags) < 0) {
		errno = EINVAL;
		return NULL;
	}

	fd = zvfs_open(name, flags, FOPEN_CREATE_MODE);
	if (fd < 0) {
		return NULL;
	}

	fp = zvfs_libc_fdopen(fd, mode);
	if (fp == NULL) {
		(void)zvfs_close(fd);
		return NULL;
	}

	return fp;
}

FILE *freopen(const char *name, const char *mode, FILE *stream)
{
	if (stream != NULL) {
		int fd = zvfs_libc_fileno(stream);

		if (fd >= 0) {
			(void)zvfs_close(fd);
		}
	}

	if (name == NULL) {
		errno = ENOTSUP;
		return NULL;
	}

	return fopen(name, mode);
}
