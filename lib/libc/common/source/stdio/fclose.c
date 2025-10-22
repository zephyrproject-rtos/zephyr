/*
 * Copyright (c) 2026 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/fdtable.h>
#include <stdio.h>

int fileptr_to_fd(FILE *stream);

int fclose(FILE *stream)
{
	int fd = fileptr_to_fd(stream);

	if (fd < 0) {
		return EOF;
	}

	int rc = zvfs_close(fd);

	if (rc < 0) {
		return EOF;
	}
	return 0;
}
