/*
 * Copyright (c) 2026 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/fdtable.h>
#include <zephyr/sys/libc-hooks.h>
#include <stdio.h>

int fileptr_to_fd(FILE *stream);

int fputc(int c, FILE *stream)
{
	int fd = fileptr_to_fd(stream);

	if (fd < 0) {
		return EOF;
	}
	unsigned char cc = c;
	int rc = zvfs_write(fd, &cc, sizeof(cc), NULL);

	if (rc <= 0) {
		return EOF;
	}
	return c;
}
