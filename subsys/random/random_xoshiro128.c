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

#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/kernel.h>
#include <string.h>

static const struct device *const entropy_driver =
	DEVICE_DT_GET(DT_CHOSEN(zephyr_entropy));
static uint32_t state[4];
static bool initialized;

static inline uint32_t rotl(const uint32_t x, int k)
{
	return (x << k) | (x >> (32 - k));
}

static int xoshiro128_initialize(void)
{
	if (!device_is_ready(entropy_driver)) {
		return -ENODEV;
	}
	return 0;
}

static void xoshiro128_init_state(void)
{
	int rc;

	/* This is not thread safe but it doesn't matter as we will just end
	 * up with a mix of random bytes from both threads.
	 */
	rc = entropy_get_entropy(entropy_driver, (uint8_t *)&state, sizeof(state));
	if (rc == 0) {
		initialized = true;
	} else {
		/* Entropy device failed or is not yet ready.
		 * Reseed the PRNG state with pseudo-random data until it can
		 * be properly seeded. This may be needed if random numbers are
		 * requested before the backing entropy device has been enabled.
		 */
		state[0] = k_cycle_get_32();
		state[1] = k_cycle_get_32() ^ 0x9b64c2b0;
		state[2] = k_cycle_get_32() ^ 0x86d3d2d4;
		state[3] = k_cycle_get_32() ^ 0xa00ae278;
	}
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

void z_impl_sys_rand_get(void *dst, size_t outlen)
{
	size_t blocks = outlen / sizeof(uint32_t);
	size_t rem = (outlen - (blocks * sizeof(uint32_t)));
	uint32_t *unaligned = dst;
	uint32_t ret;

	if (unlikely(!initialized)) {
		xoshiro128_init_state();
	}

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
