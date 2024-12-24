/*
 * Copyright (c) 2024 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

void flockfile(FILE *file)
{
	ARG_UNUSED(file);
}

int ftrylockfile(FILE *file)
{
	ARG_UNUSED(file);
}

void funlockfile(FILE *file)
{
	ARG_UNUSED(file);
}

int getc_unlocked(FILE *stream)
{
	printk("would read a char from FILE * %p\n", stream);
	return EOF;
}

int getchar_unlocked(void)
{
	return getc_unlocked(stdin);
}

int putc_unlocked(int c, FILE *stream)
{
	ARG_UNUSED(c);
	ARG_UNUSED(stream);

	return c;
}

int putchar_unlocked(int c)
{
	return putc_unlocked(c, stdout);
}
