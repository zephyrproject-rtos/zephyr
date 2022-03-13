/*
 * Copyright (c) 2021, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * SPDX-License-Identifier: CC0-1.0
 *
 * Based on code written in 2019 by David Blackman and Sebastiano Vigna
 * (vigna@acm.org)
 *
 * To the extent possible under law, the author has dedicated all copyright
 * and related and neighboring rights to this software to the public domain
 * worldwide. This software is distributed without any warranty.
 *
 * See <http://creativecommons.org/publicdomain/zero/1.0/>.
 *
 * From: https://prng.di.unimi.it/xoshiro128plusplus.c
 *
 * This is xoshiro128++ 1.0, one of our 32-bit all-purpose, rock-solid
 * generators. It has excellent speed, a state size (128 bits) that is
 * large enough for mild parallelism, and it passes all tests we are aware
 * of.
 *
 * For generating just single-precision (i.e., 32-bit) floating-point
 * numbers, xoshiro128+ is even faster.
 *
 * The state must be seeded so that it is not everywhere zero.
 */

#include <init.h>
#include <device.h>
#include <drivers/entropy.h>
#include <kernel.h>
#include <string.h>

static uint32_t state[4];

static inline uint32_t rotl(const uint32_t x, int k)
{
	return (x << k) | (x >> (32 - k));
}

static int xoshiro128_initialize(const struct device *dev)
{
	dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_entropy));
	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	int32_t rc = entropy_get_entropy_isr(dev, (uint8_t *)&state,
					     sizeof(state), ENTROPY_BUSYWAIT);

	if (rc == -ENOTSUP) {
		/* Driver does not provide an ISR-specific API, assume it can
		 * be called from ISR context
		 */
		rc = entropy_get_entropy(dev, (uint8_t *)&state, sizeof(state));
	}

	if (rc < 0) {
		return -EINVAL;
	}

	return 0;
}

static uint32_t xoshiro128_next(void)
{
	const uint32_t result = rotl(state[0] + state[3], 7) + state[0];

	const uint32_t t = state[1] << 9;

	state[2] ^= state[0];
	state[3] ^= state[1];
	state[1] ^= state[2];
	state[0] ^= state[3];

	state[2] ^= t;

	state[3] = rotl(state[3], 11);

	return result;
}

uint32_t z_impl_sys_rand32_get(void)
{
	uint32_t ret;

	ret = xoshiro128_next();

	return ret;
}

void z_impl_sys_rand_get(void *dst, size_t outlen)
{
	size_t blocks = outlen / sizeof(uint32_t);
	size_t rem = (outlen - (blocks * sizeof(uint32_t)));
	uint32_t *unaligned = dst;
	uint32_t ret;

	/* Write all full 32bit chunks */
	while (blocks--) {
		UNALIGNED_PUT(xoshiro128_next(), unaligned++);
	}
	/* Write trailing bytes */
	if (rem) {
		ret = xoshiro128_next();
		memcpy(unaligned, &ret, rem);
	}
}

/* In-tree entropy drivers will initialize in PRE_KERNEL_1; ensure that they're
 * initialized properly before initializing ourselves.
 */
SYS_INIT(xoshiro128_initialize, PRE_KERNEL_2,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
