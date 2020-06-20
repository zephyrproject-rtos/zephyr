/*
 * Copyright (c) 2020 Linaro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

void main(void)
{
	BUILD_ASSERT(sizeof(u8_t) == 1, "sizeof u8_t mismatch");
	BUILD_ASSERT(sizeof(u16_t) == 2, "sizeof u16_t mismatch");
	BUILD_ASSERT(sizeof(u32_t) == 4, "sizeof u32_t mismatch");
	BUILD_ASSERT(sizeof(u64_t) == 8, "sizeof u64_t mismatch");

	BUILD_ASSERT(sizeof(s8_t) == 1, "sizeof s8_t mismatch");
	BUILD_ASSERT(sizeof(s16_t) == 2, "sizeof s16_t mismatch");
	BUILD_ASSERT(sizeof(s32_t) == 4, "sizeof s32_t mismatch");
	BUILD_ASSERT(sizeof(s64_t) == 8, "sizeof s64_t mismatch");
}
