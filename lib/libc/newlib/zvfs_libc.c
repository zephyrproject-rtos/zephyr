/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * newlib implementation of the ZVFS FILE<->fd hooks.
 *
 * zvfs_libc_fdopen() pulls a raw, FILE-sized slot from the generic ZVFS store
 * and initializes it in place as an unbuffered (__SNBF) newlib stream whose I/O
 * is routed to the ZVFS read/write/lseek/close primitives via small callbacks
 * matching newlib's FILE function-pointer signatures (the fd is carried in the
 * cookie). A recursive lock is embedded in the same slot so that no dynamic
 * allocation is required, and the store never calls up into the C library.
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <reent.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/zvfs_libc.h>
#include <zephyr/toolchain.h>

/* FIXME: do not use ssize_t or off_t (POSIX -> Zephyr -> POSIX dependency cycle) */
ssize_t zvfs_read(int fd, void *buf, size_t sz, const size_t *from_offset);
ssize_t zvfs_write(int fd, const void *buf, size_t sz, const size_t *from_offset);
off_t zvfs_lseek(int fd, off_t offset, int whence);
int zvfs_close(int fd);

#ifndef __SINGLE_THREAD__
struct zvfs_newlib_file {
	FILE file;
	struct k_mutex lock;
};
#define ZVFS_NEWLIB_FILE_MIN (sizeof(struct zvfs_newlib_file))
#define ZVFS_NEWLIB_FILE_ALN (__alignof(struct zvfs_newlib_file))
#else
#define ZVFS_NEWLIB_FILE_MIN (sizeof(FILE))
#define ZVFS_NEWLIB_FILE_ALN (__alignof(FILE))
#endif

BUILD_ASSERT(ZVFS_LIBC_FILE_SIZE >= ZVFS_NEWLIB_FILE_MIN,
	     "ZVFS_LIBC_FILE_SIZE must be at least the size of a newlib FILE "
	     "(plus its embedded lock)");
BUILD_ASSERT(ZVFS_LIBC_FILE_ALIGN >= ZVFS_NEWLIB_FILE_ALN,
	     "ZVFS_LIBC_FILE_ALIGN must be at least the alignment of a newlib FILE");

static _READ_WRITE_RETURN_TYPE zvfs_read_cb(struct _reent *r, void *cookie, char *buf,
					    _READ_WRITE_BUFSIZE_TYPE n)
{
	ARG_UNUSED(r);

	return zvfs_read((int)(intptr_t)cookie, buf, n, NULL);
}

static _READ_WRITE_RETURN_TYPE zvfs_write_cb(struct _reent *r, void *cookie, const char *buf,
					     _READ_WRITE_BUFSIZE_TYPE n)
{
	ARG_UNUSED(r);

	return zvfs_write((int)(intptr_t)cookie, buf, n, NULL);
}

static _fpos_t zvfs_seek_cb(struct _reent *r, void *cookie, _fpos_t offset, int whence)
{
	ARG_UNUSED(r);

	return zvfs_lseek((int)(intptr_t)cookie, offset, whence);
}

static int zvfs_close_cb(struct _reent *r, void *cookie)
{
	ARG_UNUSED(r);

	return zvfs_close((int)(intptr_t)cookie);
}

static int sflags(const char *mode)
{
	int flags = 0;

	if (mode == NULL) {
		return 0;
	}

	switch (mode[0]) {
	case 'r':
		flags = __SRD;
		break;
	case 'w':
	case 'a':
		flags = __SWR;
		break;
	default:
		return 0;
	}

	for (const char *m = mode + 1; *m != '\0'; m++) {
		if (*m == '+') {
			flags = __SRW;
		}
	}

	return flags;
}

FILE *zvfs_libc_fdopen(int fd, const char *mode)
{
	FILE *fp = zvfs_libc_file_alloc(fd);

	if (fp == NULL) {
		errno = EMFILE;
		return NULL;
	}

	/* store memory is not guaranteed to be zeroed */
	memset(fp, 0, sizeof(*fp));

	fp->_flags = sflags(mode) | __SNBF;
	fp->_file = fd;
	fp->_cookie = (void *)(intptr_t)fd;
	fp->_data = _REENT;

	fp->_read = zvfs_read_cb;
	fp->_write = zvfs_write_cb;
	fp->_seek = zvfs_seek_cb;
	fp->_close = zvfs_close_cb;

#ifndef __SINGLE_THREAD__
	struct zvfs_newlib_file *const nf = (struct zvfs_newlib_file *)fp;

	k_mutex_init(&nf->lock);
	fp->_lock = (_flock_t)(void *)&nf->lock;
#endif

	return fp;
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

	return fp->_file;
}
