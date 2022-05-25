/*
 * Copyright (c) 2021 Space Cubics, LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/sys/libc-hooks.h>
#include <zephyr/app_memory/app_memdomain.h>

#define LIBC_DATA K_APP_DMEM(z_libc_partition)

#define OUTPUT_BITS (0x7fffffffU)
#define MULTIPLIER (1103515245U)
#define INCREMENT (12345U)

static LIBC_DATA unsigned int srand_seed = 1;

int rand_r(unsigned int *seed)
{
	*seed = (MULTIPLIER * *seed + INCREMENT) & OUTPUT_BITS;

	return *seed;
}

void srand(unsigned int s)
{
	srand_seed = s;
}

int rand(void)
{
	return rand_r(&srand_seed);
}
