/*
 * Copyright (c) 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>

#include <zephyr/sys/util.h>

int getc_unlocked(FILE *stream)
{
	ARG_UNUSED(stream);

	errno = ENOSYS;

	return EOF;
}

int getchar_unlocked(void)
{
	return getc_unlocked(stdin);
}
