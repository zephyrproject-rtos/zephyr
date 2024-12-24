/*
 * Copyright (c) 2024 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <zephyr/sys/fdtable.h>

extern void *_read_r;
extern void *_write_r;
extern void *_lseek_r;
extern void *_close_r;

static int z_libc_sflags(const char *mode)
{
	int ret = 0;

	switch (mode[0]) {
	case 'r':
		ret = ZVFS_O_RDONLY;
		break;

	case 'w':
		ret = ZVFS_O_WRONLY | ZVFS_O_CREAT | ZVFS_O_TRUNC;
		break;

	case 'a':
		ret = ZVFS_O_WRONLY | ZVFS_O_CREAT | ZVFS_O_APPEND;
		break;
	default:
		return 0;
	}

	while (*++mode) {
		switch (*mode) {
		case '+':
			ret |= ZVFS_O_RDWR;
			break;
		case 'x':
			ret |= ZVFS_O_EXCL;
			break;
		default:
			break;
		}
	}

	return ret;
}

FILE *z_libc_file_alloc(int fd, const char *mode)
{
	FILE *fp;

	fp = calloc(1, sizeof(*fp));
	if (fp == NULL) {
		return NULL;
	}

	fp->_flags = z_libc_sflags(mode);
	fp->_file = fd;
	fp->_cookie = (void *)fp;

	*((void **)fp->_read) = _read_r;
	*((void **)fp->_write) = _write_r;
	*((void **)fp->_seek) = _lseek_r;
	*((void **)fp->_close) = _close_r;

	return fp;
}

int z_libc_file_get_fd(FILE *fp)
{
	if (fp == NULL) {
		return -EINVAL;
	}

	return fp->_file;
}
