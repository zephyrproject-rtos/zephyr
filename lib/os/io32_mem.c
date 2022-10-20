/*
 * Copyright (c) 2021 Pete Dietl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/__assert.h>

#define IS_ALIGNED(X) (!((uintptr_t)X & (sizeof(uint32_t) - 1)))

volatile void *
io32_memcpy(volatile void * dst0, const volatile void * src0, size_t len)
{
	__ASSERT(len % 4 == 0, "len must be a multiple of 4!");

	volatile uint32_t *dst = dst0;
	const volatile uint32_t *src = src0;

	__ASSERT(IS_ALIGNED(dst), "dst0 must be aligned to a multiple of 4!");
	__ASSERT(IS_ALIGNED(src), "src0 must be aligned to a multiple of 4!");

	if (!((len % 4 == 0) && IS_ALIGNED(src) && IS_ALIGNED(dst))) {
		return NULL;
	}

	len /= sizeof(uint32_t);

	while (len--) {
		*dst = *src;
		++dst;
		++src;
	}

	return dst0;
}
