/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>

#if defined(CONFIG_ZVFS)
#include <zephyr/sys/zvfs.h>
#include <zephyr/sys/zvfs_libc.h>

#include "stdio_fd.h"
#endif

static int minimal_stream_fd(FILE *stream)
{
#if defined(CONFIG_ZVFS)
	if (stream == NULL) {
		errno = EBADF;
		return -1;
	}

	return zvfs_libc_fileno(stream);
#else
	ARG_UNUSED(stream);

	errno = EBADF;
	return -1;
#endif
}

void clearerr(FILE *stream)
{
	ARG_UNUSED(stream);
}

int fseek(FILE *stream, long offset, int whence)
{
#if defined(CONFIG_ZVFS)
	int fd;
	off_t pos;

	fd = minimal_stream_fd(stream);
	if (fd < 0) {
		return -1;
	}

	if (fd < MINIMAL_STDIO_FD_MIN) {
		errno = ESPIPE;
		return -1;
	}

	pos = zvfs_lseek(fd, offset, whence);

	return (pos == (off_t)-1) ? -1 : 0;
#else
	ARG_UNUSED(offset);
	ARG_UNUSED(whence);

	(void)minimal_stream_fd(stream);

	return -1;
#endif
}

long ftell(FILE *stream)
{
#if defined(CONFIG_ZVFS)
	int fd;
	off_t pos;

	fd = minimal_stream_fd(stream);
	if (fd < 0) {
		return -1L;
	}

	if (fd < MINIMAL_STDIO_FD_MIN) {
		errno = ESPIPE;
		return -1L;
	}

	pos = zvfs_lseek(fd, 0, SEEK_CUR);
	if (pos == (off_t)-1) {
		return -1L;
	}

	return (long)pos;
#else
	(void)minimal_stream_fd(stream);

	return -1L;
#endif
}

void rewind(FILE *stream)
{
	(void)fseek(stream, 0L, SEEK_SET);
	clearerr(stream);
}

int fgetpos(FILE *ZRESTRICT stream, fpos_t *ZRESTRICT pos)
{
	long loc;

	if (stream == NULL || pos == NULL) {
		errno = EBADF;
		return -1;
	}

	loc = ftell(stream);
	if (loc == -1L) {
		return -1;
	}

	*pos = (fpos_t)loc;

	return 0;
}

int fsetpos(FILE *stream, const fpos_t *pos)
{
	if (stream == NULL || pos == NULL) {
		errno = EBADF;
		return -1;
	}

	return fseek(stream, (long)*pos, SEEK_SET);
}
