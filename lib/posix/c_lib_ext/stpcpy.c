/*
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <string.h>

#include <zephyr/toolchain.h>

char *stpncpy(char *ZRESTRICT s1, const char *ZRESTRICT s2, size_t n)
{
	char *ret = NULL;

	for (; n != 0; ++s1, ++s2, --n) {
		*s1 = *s2;
		if (*s2 == '\0') {
			ret = s1;
		}
	}

	for (; n != 0; --n) {
		*s1++ = '\0';
	}

	if (ret == NULL) {
		ret = s1;
	}

	return ret;
}

char *stpcpy(char *ZRESTRICT s1, const char *ZRESTRICT s2)
{
	return stpncpy(s1, s2, strlen(s2) + 1);
}
