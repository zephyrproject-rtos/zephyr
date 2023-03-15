/* SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 * Copyright (c) 2017  SiFive Inc. All rights reserved.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the FreeBSD License.   This program is distributed in the hope that
 * it will be useful, but WITHOUT ANY WARRANTY expressed or implied,
 * including the implied warranties of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.  A copy of this license is available at
 * http://www.opensource.org/licenses.
 */

#include <limits.h>
#include <string.h>
#include <stdint.h>

#if LONG_MAX == 2147483647L
static inline unsigned long __detect_null(unsigned long X)
{
	return ((X)-0x01010101) & ~(X)&0x80808080;
}
#else
#if LONG_MAX == 9223372036854775807L
static inline unsigned long __detect_null(unsigned long X)
{
	return ((X)-0x0101010101010101) & ~(X)&0x8080808080808080;
}
#else
#error long int is not a 32bit or 64bit type.
#endif
#endif

size_t strlen(const char *str)
{
	const char *start = str;

#if defined(PREFER_SIZE_OVER_SPEED) || defined(__OPTIMIZE_SIZE__)
	while (*str++) {
		;
	}
	return str - start - 1;
#else
	if (__builtin_expect((uintptr_t)str & (sizeof(long) - 1), 0)) {
		do {
			char ch = *str;

			str++;
			if (!ch) {
				return str - start - 1;
			}
		} while ((uintptr_t)str & (sizeof(long) - 1));
	}

	unsigned long *ls = (unsigned long *)str;
	while (!__detect_null(*ls++)) {
		;
	}
	__asm__ volatile("" : "+r"(ls)); /* prevent "optimization" */

	str = (const char *)ls;

	size_t ret = str - start, sl = sizeof(long);
	char c0 = str[0 - sl], c1 = str[1 - sl], c2 = str[2 - sl], c3 = str[3 - sl];

	if (c0 == 0) {
		return ret + 0 - sl;
	}
	if (c1 == 0) {
		return ret + 1 - sl;
	}
	if (c2 == 0) {
		return ret + 2 - sl;
	}
	if (sl == 4 || c3 == 0) {
		return ret + 3 - sl;
	}

	c0 = str[4 - sl], c1 = str[5 - sl], c2 = str[6 - sl], c3 = str[7 - sl];
	if (c0 == 0) {
		return ret + 4 - sl;
	}
	if (c1 == 0) {
		return ret + 5 - sl;
	}
	if (c2 == 0) {
		return ret + 6 - sl;
	}

	return ret + 7 - sl;
#endif /* not PREFER_SIZE_OVER_SPEED */
}
