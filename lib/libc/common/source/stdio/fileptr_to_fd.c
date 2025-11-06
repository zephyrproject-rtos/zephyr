/*
 * Copyright (c) 2026 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/sys/zvfs_fd_file.h>

int fileptr_to_fd(FILE *stream)
{
	int fd = -1;

	if (stream == stdin) {
		fd = 0;
	} else if (stream == stdout) {
		fd = 1;
	} else if (stream == stderr) {
		fd = 2;
	} else {
		fd = zvfs_fileno(stream);
	}

	return fd;
}
