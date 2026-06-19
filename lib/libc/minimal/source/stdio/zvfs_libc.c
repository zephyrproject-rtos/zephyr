/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Minimal libc implementation of the ZVFS FILE<->fd hooks.
 *
 * Minimal libc has no real FILE object: FILE is an int and the standard streams
 * are the literal pointers (FILE *)1/2/3 (see lib/libc/minimal/include/stdio.h).
 * ZVFS-backed streams encode their fd as (fd + 4) so they never alias stdin/stdout/
 * stderr when the fdtable does not pre-reserve stdio slots
 * (CONFIG_ZVFS_RESERVE_STDIO_FDS=n).
 * No backing store is required.
 */

#include <stdint.h>
#include <stdio.h>

#include <zephyr/sys/zvfs_libc.h>

#define MINIMAL_LIBC_STDIO_FILE_BASE 4

FILE *zvfs_libc_fdopen(int fd, const char *mode)
{
	ARG_UNUSED(mode);

	return (FILE *)(intptr_t)(fd + MINIMAL_LIBC_STDIO_FILE_BASE);
}

int zvfs_libc_fileno(FILE *fp)
{
	intptr_t stream = (intptr_t)fp;

	if (stream >= 1 && stream <= 3) {
		return (int)stream - 1;
	}

	return (int)stream - MINIMAL_LIBC_STDIO_FILE_BASE;
}
