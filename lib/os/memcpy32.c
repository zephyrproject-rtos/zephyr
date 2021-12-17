#include <sys/util.h>
#include <sys/__assert.h>
#include <stdint.h>
#include <toolchain.h>

/*
 * Copyright (c) 2021 Pete Dietl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define IS_ALIGNED(X) (!((uintptr_t)X & (sizeof(uint32_t) - 1)))

void *memcpy32(volatile void *ZRESTRICT dst0, const volatile void *ZRESTRICT src0, size_t len)
{
	__ASSERT(len % 4 == 0, "len must be a multiple of 4!");

	typedef uint32_t __may_alias u32_alias;

	char *dst = dst0;
	const char *src = src0;

	__ASSERT(IS_ALIGNED(dst), "dest must be aligned to a multiple of 4!");
	__ASSERT(IS_ALIGNED(src), "src must be aligned to a multiple of 4!");

	if ((len % 4 == 0) && IS_ALIGNED(src) && IS_ALIGNED(dst)) {
		while (len >= 16) {
			*(u32_alias *)dst = *(u32_alias *)src;
			*(u32_alias *)(dst + 4) = *(u32_alias *)(src + 4);
			*(u32_alias *)(dst + 8) = *(u32_alias *)(src + 8);
			*(u32_alias *)(dst + 12) = *(u32_alias *)(src + 12);
			len -= 16;
			dst += 16;
			src += 16;
		}

		/* len is 12, 8, 4, or 0 */
		while (len > 0) {
			*(u32_alias *)dst = *(u32_alias *)src;
			len -= 4;
			dst += 4;
			src += 4;
		}

		return dst;
	} else {
		return NULL;
	}
}
