/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#if defined(CONFIG_ZVFS)
#include <zephyr/sys/zvfs.h>
#include <zephyr/sys/zvfs_libc.h>
#endif

int fgetc(FILE *stream)
{
#if defined(CONFIG_ZVFS)
	unsigned char ch;
	int fd;
	long ret;

	if (stream == NULL) {
		return EOF;
	}

	fd = zvfs_libc_fileno(stream);
	if (fd < 3) {
		return EOF;
	}

	ret = zvfs_read(fd, &ch, 1);
	if (ret != 1) {
		return EOF;
	}

	return (int)ch;
#else
	ARG_UNUSED(stream);

	return EOF;
#endif
}
