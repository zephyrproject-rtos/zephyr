/*
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

char *strndup(const char *s, size_t n)
{
	size_t len = strlen(s);
	char *ret = malloc(n);

	if (ret != NULL) {
		strncpy(ret, s, n);
		ret[n - 1] = '\0';
	}

	return ret;
}

char *strdup(const char *s)
{
	return strndup(s, strlen(s) + 1);
}
