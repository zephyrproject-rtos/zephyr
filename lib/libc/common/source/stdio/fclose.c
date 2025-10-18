/*
 * Copyright (c) 2025 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/fdtable.h>
#include <unistd.h>

int fclose(FILE *stream)
{
	int fd;

	fflush(stream);
	fd = zvfs_libc_file_get_fd(stream);

	if (!fd) {
		return EOF;
	}
	if (close(fd)) {
		return EOF;
	}

	return 0;
}
