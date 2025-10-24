/*
 * Copyright (c) 2026 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/fdtable.h>
#include <stdio.h>

/* in case feof is a macro */
#ifdef feof
#undef feof
#endif

int fileptr_to_fd(FILE *stream);

int feof(FILE *stream)
{
	int fd = fileptr_to_fd(stream);

	if (fd < 0) {
		return EOF;
	}
	int cur = zvfs_lseek(fd, 0, FS_SEEK_CUR);

	if (cur < 0) {
		return cur;
	}
	int end = zvfs_lseek(fd, 0, FS_SEEK_END);

	if (end < 0) {
		return end;
	}
	return cur == end;
}
