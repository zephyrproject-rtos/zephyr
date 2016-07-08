/*
 * Copyright (c) 2011-2014, Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 * @brief Misc utilities
 *
 * Misc utilities usable by nanokernel, microkernel, and application code.
 */

#ifndef _UTIL__H_
#define _UTIL__H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE

#include <stdint.h>

/* Helper to pass a int as a pointer or vice-versa.
 * Those are available for 32 bits architectures:
 */
#define POINTER_TO_UINT(x) ((uint32_t) (x))
#define UINT_TO_POINTER(x) ((void *) (x))
#define POINTER_TO_INT(x)  ((int32_t) (x))
#define INT_TO_POINTER(x)  ((void *) (x))

#define ARRAY_SIZE(array) ((unsigned long)(sizeof(array) / sizeof((array)[0])))
#define CONTAINER_OF(ptr, type, field) \
	((type *)(((char *)(ptr)) - offsetof(type, field)))

/* round "x" up/down to next multiple of "align" (which must be a power of 2) */
#define ROUND_UP(x, align)                                   \
	(((unsigned long)(x) + ((unsigned long)align - 1)) & \
	 ~((unsigned long)align - 1))
#define ROUND_DOWN(x, align) ((unsigned long)(x) & ~((unsigned long)align - 1))

#ifdef INLINED
#define INLINE inline
#else
#define INLINE
#endif

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

static inline int is_power_of_two(unsigned int x)
{
	return (x != 0) && !(x & (x - 1));
}

static inline int64_t arithmetic_shift_right(int64_t value, uint8_t shift)
{
	int64_t sign_ext;

	if (shift == 0) {
		return value;
	}

	/* extract sign bit */
	sign_ext = (value >> 63) & 1;

	/* make all bits of sign_ext be the same as the value's sign bit */
	sign_ext = -sign_ext;

	/* shift value and fill opened bit positions with sign bit */
	return (value >> shift) | (sign_ext << (64 - shift));
}

#endif /* !_ASMLANGUAGE */

/* KB, MB, GB */
#define KB(x) ((x) << 10)
#define MB(x) (KB(x) << 10)
#define GB(x) (MB(x) << 10)

/* KHZ, MHZ */
#define KHZ(x) ((x) * 1000)
#define MHZ(x) (KHZ(x) * 1000)

#ifndef BIT
#define BIT(n)  (1UL << (n))
#endif

#define BIT_MASK(n) (BIT(n) - 1)

#ifdef __cplusplus
}
#endif

#endif /* _UTIL__H_ */
