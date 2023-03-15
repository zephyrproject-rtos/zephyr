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

char *strcpy(char *dst, const char *src)
{
	char *dst0 = dst;

#if !defined(PREFER_SIZE_OVER_SPEED) && !defined(__OPTIMIZE_SIZE__)
	int misaligned = ((uintptr_t)dst | (uintptr_t)src) & (sizeof(long) - 1);

	if (__builtin_expect(!misaligned, 1)) {
		const long *lsrc = (const long *)src;
		long *ldst = (long *)dst;

		while (!__detect_null(*lsrc)) {
			*ldst++ = *lsrc++;
		}

		dst = (char *)ldst;
		src = (const char *)lsrc;

		char c0 = src[0];
		char c1 = src[1];
		char c2 = src[2];

		if (!(*dst++ = c0)) {
			return dst0;
		}
		if (!(*dst++ = c1)) {
			return dst0;
		}

		char c3 = src[3];

		if (!(*dst++ = c2)) {
			return dst0;
		}
		if (sizeof(long) == 4) {
			goto out;
		}

		char c4 = src[4];

		if (!(*dst++ = c3)) {
			return dst0;
		}

		char c5 = src[5];

		if (!(*dst++ = c4)) {
			return dst0;
		}

		char c6 = src[6];

		if (!(*dst++ = c5)) {
			return dst0;
		}
		if (!(*dst++ = c6)) {
			return dst0;
		}

	out:
		*dst++ = 0;

		return dst0;
	}
#endif /* not PREFER_SIZE_OVER_SPEED */

	char ch;

	do {
		ch = *src;
		src++;
		dst++;
		*(dst - 1) = ch;
	} while (ch);

	return dst0;
}
