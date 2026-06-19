/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/sys/zvfs.h>
#include <zephyr/sys/zvfs_libc.h>

int fclose(FILE *stream)
{
	int fd;

	if (stream == NULL) {
		return EOF;
	}

	(void)fflush(stream);

	fd = zvfs_libc_fileno(stream);
	if (fd < 0) {
		return EOF;
	}

	if (zvfs_close(fd) < 0) {
		return EOF;
	}
	zvfs_libc_file_free(stream);

	return 0;
}
