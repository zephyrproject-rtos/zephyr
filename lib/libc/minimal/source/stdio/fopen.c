/* stdout_console.c */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <misc/libc-hooks.h>

size_t _impl__zephyr_fopen(const char *path, const char *mode)
{
	size_t i;
	size_t j;
	const unsigned char *p;

	if ((stream != stdout) || (nitems == 0) || (size == 0)) {
		return 0;
	}

	p = ptr;
	i = nitems;
	do {
		j = size;
		do {
			if (_stdout_hook((int) *p++) == EOF) {
				goto done;
			}
			j--;
		} while (j > 0);

		i--;
	} while (i > 0);

done:
	return (nitems - i);
}


#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER(_zephyr_fopen, path, mode)
{

	Z_OOPS(Z_SYSCALL_MEMORY_ARRAY_READ(ptr, nitems, size));
	return _impl__zephyr_fopen((const char *)path,
				   (const char *)mode);
}
#endif

size_t fopen(const char *path, const char *mode)
{
	return _zephyr_fopen(path, mode);
}
