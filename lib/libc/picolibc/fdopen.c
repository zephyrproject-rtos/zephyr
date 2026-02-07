/*
 * Copyright © 2025, Keith Packard <keithp@keithp.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio-bufio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <zephyr/sys/fdtable.h>

/* not exported by picolibc */
int
__stdio_flags(const char *mode, int *optr);

static ssize_t
_read(int fd, void *buf, size_t sz)
{
	return zvfs_read(fd, buf, sz, NULL);
}

static ssize_t
_write(int fd, const void *buf, size_t sz)
{
	return zvfs_write(fd, buf, sz, NULL);
}

FILE *
fdopen(int fd, const char *mode)
{
	int stdio_flags;
	int open_flags;
	struct __file_bufio *bf;
	char *buf;

	stdio_flags = __stdio_flags(mode, &open_flags);
	if (stdio_flags == 0) {
		return NULL;
	}

	/* Allocate file structure and necessary buffers */
	bf = calloc(1, sizeof(struct __file_bufio) + BUFSIZ);

	if (bf == NULL) {
		return NULL;
	}

	buf = (char *) (bf + 1);

	*bf = (struct __file_bufio)
		FDEV_SETUP_BUFIO(fd, buf, BUFSIZ,
				 _read,
				 _write,
				 zvfs_lseek,
				 zvfs_close,
				 stdio_flags, __BFALL);

	if (open_flags & O_APPEND) {
		(void) fseeko(&(bf->xfile.cfile.file), 0, SEEK_END);
	}

	return &(bf->xfile.cfile.file);
}
