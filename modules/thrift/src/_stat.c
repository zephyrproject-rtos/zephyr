/*
 * Copyright 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <sys/stat.h>

#include <zephyr/kernel.h>

int stat(const char *restrict path, struct stat *restrict buf)
{
	ARG_UNUSED(path);
	ARG_UNUSED(buf);

	errno = ENOTSUP;

	return -1;
}
