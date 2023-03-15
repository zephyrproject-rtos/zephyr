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

#if defined(PREFER_SIZE_OVER_SPEED) || defined(__OPTIMIZE_SIZE__)
/* memcpy defined in memcpy-asm.S */
#else

#include <string.h>
#include <stdint.h>

void *memcpy(void *__restrict aa, const void *__restrict bb, size_t n)
{
#define BODY(a, b, t)                                                                              \
	{                                                                                          \
		t tt = *b;                                                                         \
		a++, b++;                                                                          \
		*(a - 1) = tt;                                                                     \
	}

	char *a = (char *)aa;
	const char *b = (const char *)bb;
	char *end = a + n;
	uintptr_t msk = sizeof(long) - 1;

	if (unlikely((((uintptr_t)a & msk) != ((uintptr_t)b & msk)) || n < sizeof(long))) {
	small:
		if (__builtin_expect(a < end, 1)) {
			while (a < end) {
				BODY(a, b, char);
			}
		}
		return aa;
	}

	if (unlikely(((uintptr_t)a & msk) != 0)) {
		while ((uintptr_t)a & msk) {
			BODY(a, b, char);
		}
	}

	long *la = (long *)a;
	const long *lb = (const long *)b;
	long *lend = (long *)((uintptr_t)end & ~msk);

	if (unlikely(lend - la > 8)) {
		while (lend - la > 8) {
			long b0 = *lb++;
			long b1 = *lb++;
			long b2 = *lb++;
			long b3 = *lb++;
			long b4 = *lb++;
			long b5 = *lb++;
			long b6 = *lb++;
			long b7 = *lb++;
			long b8 = *lb++;

			*la++ = b0;
			*la++ = b1;
			*la++ = b2;
			*la++ = b3;
			*la++ = b4;
			*la++ = b5;
			*la++ = b6;
			*la++ = b7;
			*la++ = b8;
		}
	}

	while (la < lend) {
		BODY(la, lb, long);
	}

	a = (char *)la;
	b = (const char *)lb;
	if (unlikely(a < end)) {
		goto small;
	}
	return aa;
}
#endif
