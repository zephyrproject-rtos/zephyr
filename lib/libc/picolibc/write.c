/*
 * Copyright © 2021, Keith Packard <keithp@keithp.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/libc-hooks.h>

/* Don't replace the version supplied by fdtable */
#if !defined(CONFIG_FDTABLE) || !defined(CONFIG_POSIX_API)
ssize_t write(int fd, const void *buf, size_t len)
{
	size_t count;
	const char *b = buf;

	(void) fd;
	for (count = 0; count < len; count++)
		zephyr_fputc(*b++, stdout);
	return len;
}
#endif
