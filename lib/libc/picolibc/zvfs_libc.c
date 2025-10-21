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
#include <zephyr/toolchain.h>

BUILD_ASSERT(CONFIG_ZVFS_LIBC_FILE_SIZE >= sizeof(struct __file_bufio),
	     "CONFIG_ZVFS_LIBC_FILE_SIZE must at least match size of FILE object");
BUILD_ASSERT(CONFIG_ZVFS_LIBC_FILE_ALIGN == __alignof(struct __file_bufio),
	     "CONFIG_ZVFS_LIBC_FILE_SIZE must match alignment of FILE object");

#define FDEV_SETUP_ZVFS(fd, buf, size, rwflags, bflags)                                            \
	FDEV_SETUP_BUFIO(fd, buf, size, zvfs_read_wrap, zvfs_write_wrap, zvfs_lseek, zvfs_close,   \
			 rwflags, bflags)

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

static int sflags(const char *mode)
{
	int flags = 0;

	if (mode == NULL) {
		return 0;
	}

	switch (mode[0]) {
	case 'r':
		flags |= __SRD;
		break;
	case 'w':
		flags |= __SWR;
		break;
	case 'a':
		flags |= __SWR;
		break;
	default:
		/* ignore */
		break;
	}
	if (mode[1] == '+') {
		flags |= __SRD | __SWR;
	}

	return flags;
}

void zvfs_libc_file_alloc_cb(int fd, const char *mode, FILE *fp)
{
	struct __file_bufio *const bf = (struct __file_bufio *)fp;

	*bf = (struct __file_bufio)FDEV_SETUP_ZVFS(fd, (char *)(bf + 1), BUFSIZ, sflags(mode),
						   __BFALL);

	__bufio_lock_init(&(bf->xfile.cfile.file));
}

int zvfs_libc_file_get_fd(FILE *fp)
{
	const struct __file_bufio *const bf = (const struct __file_bufio *)fp;

	return POINTER_TO_INT(bf->ptr);
}
