/*
 * Copyright (c) 2026 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/fdtable.h>
#include <stdio.h>

int fileptr_to_fd(FILE *stream);

size_t fread(void *ZRESTRICT ptr, size_t size, size_t nmemb, FILE *ZRESTRICT stream)
{
	int fd = fileptr_to_fd(stream);

	if (fd < 0) {
		return 0;
	}
	unsigned char *buf = ptr;
	int i = 0;

	for (; i < nmemb; i++) {
		int rc = zvfs_read(fd, buf + (size * i), size, NULL);

		if (rc <= 0) {
			return i;
		}
	}
	return i;
}
