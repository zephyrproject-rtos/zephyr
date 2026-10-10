/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <ctype.h>

int strcasecmp(const char *s1, const char *s2)
{
	unsigned char c = 1U;
	int result = 0;

	while (c != 0U && result == 0) {
		c = *s1++;
		result = tolower(c) - tolower((unsigned char)*s2++);
	}

	return result;
}
