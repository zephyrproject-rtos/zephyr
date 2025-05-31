/*
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>
#include <stddef.h>

int strncasecmp(const char *s1, const char *s2, size_t n)
{
	int c1, c2;

	for (; n != 0; --n) {
		c1 = (unsigned char)*s1++;
		c2 = (unsigned char)*s2++;
		if (c1 == '\0') {
			return c1 - c2;
		}
		if (c2 == '\0') {
			return c1 - c2;
		}

		c1 = tolower(c1);
		c2 = tolower(c2);

		if (c1 != c2) {
			return c1 - c2;
		}
	}

	return 0;
}

int strcasecmp(const char *s1, const char *s2)
{
	return strncasecmp(s1, s2, MIN(strlen(s1), strlen(s2)));
}
