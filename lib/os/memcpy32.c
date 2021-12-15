#include <sys/util.h>
#include <stdint.h>
#include <toolchain.h>

/*
 * Copyright (c) 2021 Pete Dietl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

void *memcpy32(void *restrict dest, const void *restrict src, size_t n)
{
	uint8_t *d = dest;
	const uint8_t *s = src;

	typedef uint32_t __may_alias u32;

	if (s % 4 == 0 && d % 4 == 0) {
		for (; n >= 16; s += 16, d += 16, n -= 16) {
			*(u32 *)(d + 0) = *(u32 *)(s + 0);
			*(u32 *)(d + 4) = *(u32 *)(s + 4);
			*(u32 *)(d + 8) = *(u32 *)(s + 8);
			*(u32 *)(d + 12) = *(u32 *)(s + 12);
		}
		/* At this point, there could be 12, 8, 4, or 0 more bytes remaining */
		if (n & 8) {
			*(u32 *)(d + 0) = *(u32 *)(s + 0);
			*(u32 *)(d + 4) = *(u32 *)(s + 4);
			d += 8; s += 8;
		}
		if (n & 4) {
			*(u32 *)(d + 0) = *(u32 *)(s + 0);
		}
		return dest;
	} else {
		for (; n; n--) {
			*d++ = *s++;
		}
		return dest;
	}
}
