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
	unsigned char c1 = 1U;
	unsigned char c2;

	for (; c1 && n != 0; n--) {
		unsigned char lower1, lower2;

		c1 = *s1++;
		lower1 = tolower(c1);
		c2 = *s2++;
		lower2 = tolower(c2);

		if (lower1 != lower2) {
			return (lower1 > lower2) - (lower1 < lower2);
		}
	}

	return 0;
}
