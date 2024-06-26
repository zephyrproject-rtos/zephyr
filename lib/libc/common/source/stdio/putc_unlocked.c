/*
 * Copyright (c) 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/sys/libc-hooks.h>

int putc_unlocked(int c, FILE *stream)
{
	return zephyr_fputc(c, stream);
}

int putchar_unlocked(int c)
{
	return putc_unlocked(c, stdout);
}
