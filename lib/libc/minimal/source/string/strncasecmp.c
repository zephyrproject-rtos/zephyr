/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <ctype.h>

int
strncasecmp(const char *s1, const char *s2, size_t n)
{
	char c1 = ' ';

	for (; (c1 != '\0') && (n != 0); n--) {
		c1 = *s1++;
		char c2 = *s2++;
		int lower1 = tolower((int)(unsigned char)c1);
		int lower2 = tolower((int)(unsigned char)c2);

		if (lower1 != lower2) {
			return (lower1 > lower2) ? 1 : -1;
		}
	}

	return 0;
}
