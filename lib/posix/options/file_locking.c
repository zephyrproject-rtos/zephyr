/*
 * Copyright (c) 2024 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>

#include <zephyr/toolchain.h>
#include <zephyr/sys/fdtable.h>

int zvfs_lock_file(FILE *file, k_timeout_t timeout);
int zvfs_unlock_file(FILE *file);
int zvfs_getc_unlocked(FILE *stream);
int zvfs_putc_unlocked(int c, FILE *stream);

/*
 * This is incorrectly declared by (at least) newlib to be a macro with 2 arguments
 * but it only takes 1 argument.
 *
 * Undefine any possible macro before attempting to define a duplicately-named function.
 */
#undef putchar_unlocked

/*
 * This is incorrectly declared by (at least) newlib to be a macro with 0 arguments
 * but it should take 1 argument.
 *
 * Undefine any possible macro before attempting to define a duplicately-named function.
 */
#undef getchar_unlocked

void flockfile(FILE *file)
{
	while (zvfs_lock_file(file, K_FOREVER) != 0) {
		k_yield();
	}
}

int ftrylockfile(FILE *file)
{
	return zvfs_lock_file(file, K_NO_WAIT);
}

void funlockfile(FILE *file)
{
	(void)zvfs_unlock_file(file);
}

int getc_unlocked(FILE *stream)
{
	return zvfs_getc_unlocked(stream);
}

int getchar_unlocked(void)
{
	return zvfs_getc_unlocked(stdin);
}

int putc_unlocked(int c, FILE *stream)
{
	return zvfs_putc_unlocked(c, stream);
}

int putchar_unlocked(int c)
{
	return zvfs_putc_unlocked(c, stdout);
}
