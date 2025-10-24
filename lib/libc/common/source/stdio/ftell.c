/*
 * Copyright (c) 2026 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <errno.h>
#include <zephyr/sys/fdtable.h>

int fileptr_to_fd(FILE *stream);

long ftell(FILE *stream)
{
	int fd = fileptr_to_fd(stream);

	if (fd < 0) {
		return EOF;
	}
	return zvfs_lseek(fd, 0, FS_SEEK_CUR);
}
