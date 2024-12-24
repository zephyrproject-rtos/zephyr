/*
 * Copyright (c) 2024 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <zephyr/sys/fdtable.h>

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
	/*
	 * These symbols have conflicting declarations in upstream headers.
	 * Use uintptr_t and a cast to assign cleanly.
	 */
	extern uintptr_t _close_r;
	extern uintptr_t _lseek_r;
	extern uintptr_t _read_r;
	extern uintptr_t _write_r;

	fp = calloc(1, sizeof(*fp));
	if (fp == NULL) {
		return NULL;
	}

	fp->_flags = z_libc_sflags(mode);
	fp->_file = fd;
	fp->_cookie = (void *)fp;

	*(uintptr_t *)fp->_read = _read_r;
	*(uintptr_t *)fp->_write = _write_r;
	*(uintptr_t *)fp->_seek = _lseek_r;
	*(uintptr_t *)fp->_close = _close_r;

	return fp;
}

int z_libc_file_get_fd(FILE *fp)
{
	return fp->_file;
}
