/*
 * Copyright (c) 2024, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/posix/unistd.h>

size_t confstr(int name, char *buf, size_t len)
{
	if (name < 0 || name > _CS_V6_ENV) {
		errno = EINVAL;
		return 0;
	}

	if (buf != NULL && len > 0) {
		buf[0] = '\0';
	}

	return 1;
}
