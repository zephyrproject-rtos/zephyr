/*
 * Copyright (c) 2019 Facebook.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Inline implementation of functions declared in math_extras.h.
 */

#ifndef ZEPHYR_INCLUDE_MISC_MATH_EXTRAS_H_
#error "please include <misc/math_extras.h> instead of this file"
#endif

static inline bool u32_add_overflow(u32_t a, u32_t b, u32_t *result)
{
	u32_t c = a + b;

	*result = c;

	return c < a;
}

static inline bool u64_add_overflow(u64_t a, u64_t b, u64_t *result)
{
	u64_t c = a + b;

	*result = c;

	return c < a;
}

static inline bool size_add_overflow(size_t a, size_t b, size_t *result)
{
	size_t c = a + b;

	*result = c;

	return c < a;
}

static inline bool u32_mul_overflow(u32_t a, u32_t b, u32_t *result)
{
	u32_t c = a * b;

	*result = c;

	return a != 0 && (c / a) != b;
}

static inline bool u64_mul_overflow(u64_t a, u64_t b, u64_t *result)
{
	u64_t c = a * b;

	*result = c;

	return a != 0 && (c / a) != b;
}

static inline bool size_mul_overflow(size_t a, size_t b, size_t *result)
{
	size_t c = a * b;

	*result = c;

	return a != 0 && (c / a) != b;
}

static inline int u32_count_leading_zeros(u32_t x)
{
	int b;

	for (b = 0; b < 32 && (x >> 31) == 0; b++) {
		x <<= 1;
	}

	return b;
}

static inline int u64_count_leading_zeros(u64_t x)
{
	if (x == (u32_t)x) {
		return 32 + u32_count_leading_zeros((u32_t)x);
	} else {
		return u32_count_leading_zeros(x >> 32);
	}
}

static inline int u32_count_trailing_zeros(u32_t x)
{
	int b;

	for (b = 0; b < 32 && (x & 1) == 0; b++) {
		x >>= 1;
	}

	return b;
}

static inline int u64_count_trailing_zeros(u64_t x)
{
	if ((u32_t)x) {
		return u32_count_trailing_zeros((u32_t)x);
	} else {
		return 32 + u32_count_trailing_zeros(x >> 32);
	}
}
