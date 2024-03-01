/*
 * Copyright (c) 2024, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define __STDC_WANT_LIB_EXT1__ 1

#include <stddef.h>
#include <string.h>

#include <zephyr/kernel.h>

size_t k_strnlen(const char *s, size_t maxlen)
{
#ifdef __STDC_LIB_EXT1__
	return strnlen_s(s, maxlen);
#else
	size_t len = 0;

	for (; len < maxlen && *s != '\0'; s++, len++) {
	}

	return len;
#endif
}
