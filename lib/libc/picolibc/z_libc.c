/*
 * Copyright (c) 2024 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stdio-bufio.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <zephyr/sys/fdtable.h>

#define FDEV_SETUP_ZVFS(fd, buf, size, rwflags, bflags)                                            \
	FDEV_SETUP_BUFIO(fd, buf, size, zvfs_read_wrap, zvfs_write_wrap, zvfs_lseek, zvfs_close,   \
			 rwflags, bflags)

int __posix_sflags(const char *mode, int *optr);

/* FIXME: do not use ssize_t or off_t */
ssize_t zvfs_read(int fd, void *buf, size_t sz, const size_t *from_offset);
ssize_t zvfs_write(int fd, const void *buf, size_t sz, const size_t *from_offset);
off_t zvfs_lseek(int fd, off_t offset, int whence);
int zvfs_close(int fd);

static ssize_t zvfs_read_wrap(int fd, void *buf, size_t sz)
{
	return zvfs_read(fd, buf, sz, NULL);
}

static ssize_t zvfs_write_wrap(int fd, const void *buf, size_t sz)
{
	return zvfs_write(fd, buf, sz, NULL);
}

FILE *z_libc_file_alloc(int fd, const char *mode)
{
	struct __file_bufio *bf;
	int __maybe_unused unused;

	bf = calloc(1, sizeof(struct __file_bufio) + BUFSIZ);
	if (bf == NULL) {
		return NULL;
	}

	*bf = (struct __file_bufio)FDEV_SETUP_ZVFS(fd, (char *)(bf + 1), BUFSIZ,
						   __posix_sflags(mode, &unused), __BFALL);

	__bufio_lock_init(&(bf->xfile.cfile.file));

	return &(bf->xfile.cfile.file);
}

int z_libc_file_get_fd(const FILE *fp)
{
	const struct __file_bufio *const bf = (const struct __file_bufio *)fp;

	return POINTER_TO_INT(bf->ptr);
}
