/*
 * Copyright (c) 2021 Space Cubics, LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/sys/libc-hooks.h>

#define OUTPUT_BITS (0x7fffffffU)
#define MULTIPLIER (1103515245U)
#define INCREMENT (12345U)

int rand_r(unsigned int *seed)
{
	*seed = (MULTIPLIER * *seed + INCREMENT) & OUTPUT_BITS;

	return *seed;
}

#ifdef CONFIG_MINIMAL_LIBC_NON_REENTRANT_FUNCTIONS
static Z_LIBC_DATA unsigned int srand_seed = 1;

void srand(unsigned int s)
{
	srand_seed = s;
}

int rand(void)
{
	return rand_r(&srand_seed);
}
#endif /* CONFIG_MINIMAL_LIBC_NON_REENTRANT_FUNCTIONS */
