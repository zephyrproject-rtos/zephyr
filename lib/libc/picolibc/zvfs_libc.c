/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * picolibc (tinystdio) implementation of the ZVFS FILE<->fd hooks.
 *
 * zvfs_libc_fdopen() pulls a raw, FILE-sized slot from the generic ZVFS store
 * and initializes it in place as a tinystdio bufio stream whose backing buffer
 * lives in the same slot. I/O is routed to the ZVFS read/write/lseek/close
 * primitives. No dynamic allocation is used, and the store never calls up into
 * the C library.
 */

#include "stdio-bufio.h"

#include <errno.h>
#include <stddef.h>
#include <stdio.h>

#include <zephyr/sys/util.h>
#include <zephyr/sys/zvfs.h>
#include <zephyr/sys/zvfs_libc.h>
#include <zephyr/toolchain.h>

BUILD_ASSERT(ZVFS_LIBC_FILE_SIZE > sizeof(struct __file_bufio),
	     "ZVFS_LIBC_FILE_SIZE must exceed sizeof(struct __file_bufio) to leave room "
	     "for the stdio buffer");
BUILD_ASSERT(ZVFS_LIBC_FILE_ALIGN >= __alignof(struct __file_bufio),
	     "ZVFS_LIBC_FILE_ALIGN must be at least the alignment of struct __file_bufio");

static ssize_t zvfs_read_wrap(int fd, void *buf, size_t sz)
{
	return zvfs_read(fd, buf, sz);
}

static ssize_t zvfs_write_wrap(int fd, const void *buf, size_t sz)
{
	return zvfs_write(fd, buf, sz);
}

static __off_t zvfs_lseek_wrap(int fd, __off_t offset, int whence)
{
	return zvfs_lseek(fd, offset, whence);
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
	case 'a':
		flags |= __SWR;
		break;
	default:
		break;
	}

	for (const char *m = mode + 1; *m != '\0'; m++) {
		if (*m == '+') {
			flags |= __SRD | __SWR;
		}
	}

	return flags;
}

FILE *zvfs_libc_fdopen(int fd, const char *mode)
{
	struct __file_bufio *bf = zvfs_libc_file_alloc(fd);
	char *buf;
	int size;

	if (bf == NULL) {
		errno = EMFILE;
		return NULL;
	}

	buf = (char *)(bf + 1);
	size = (int)(ZVFS_LIBC_FILE_SIZE - sizeof(struct __file_bufio));

	/*
	 * Compound-literal init zeroes any members not named by FDEV_SETUP_BUFIO,
	 * including the (optional) per-file lock, which tinystdio then initializes
	 * lazily on first use.
	 */
	*bf = (struct __file_bufio)FDEV_SETUP_BUFIO(fd, buf, size, zvfs_read_wrap, zvfs_write_wrap,
						    zvfs_lseek_wrap, zvfs_close, sflags(mode), 0);

	return (FILE *)bf;
}

int zvfs_libc_fileno(FILE *fp)
{
	if (fp == stdin) {
		return 0;
	}
	if (fp == stdout) {
		return 1;
	}
	if (fp == stderr) {
		return 2;
	}

	return POINTER_TO_INT(((const struct __file_bufio *)fp)->ptr);
}
