/*
 * Copyright (c) 2013-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Non-random number generator based on system timer
 *
 * This module provides a non-random implementation of sys_rand32_get(), which
 * is not meant to be used in a final product as a truly random number
 * generator. It was provided to allow testing on a platform that does not (yet)
 * provide a random number generator.
 */

#include <zephyr/init.h>
#include <zephyr/random/rand32.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/device.h>

#if defined(__GNUC__)

/* seed the state with pseudo-random data */
static uint32_t state[4] = {0, 0x9b64c2b0, 0x86d3d2d4, 0xa00ae278};

static struct k_spinlock rand_state_lock;

static inline uint32_t rotl(const uint32_t x, int k)
{
	return (x << k) | (x >> (32 - k));
}

static uint32_t xoshiro128_next(void)
{
	const uint32_t result = rotl(state[0] + state[3], 7) + state[0];
	const uint32_t t = state[1] << 9;

	k_spinlock_key_t lock_key = k_spin_lock(&rand_state_lock);

	state[2] ^= state[0];
	state[3] ^= state[1];
	state[1] ^= state[2];
	state[0] ^= state[3];

	state[2] ^= t;
	state[3] = rotl(state[3], 11);

	k_spin_unlock(&rand_state_lock, lock_key);

	return result;
}

/**
 * @brief Get a 32 bit random number
 *
 * The non-random number generator returns values that are based off the
 * target's clock counter, which means that successive calls will return
 * different values.
 *
 * @return a 32-bit number
 */
uint32_t z_impl_sys_rand32_get(void)
{
	return xoshiro128_next();
}

/**
 * @brief Fill destination buffer with random numbers
 *
 * The non-random number generator returns values that are based off the
 * target's clock counter, which means that successive calls will return
 * different values.
 *
 * @param dst destination buffer to fill
 * @param outlen size of destination buffer to fill
 */
void z_impl_sys_rand_get(void *dst, size_t outlen)
{
	uint8_t *udst = dst;
	uint32_t blocksize;
	uint32_t ret;

	while (outlen) {
		ret = sys_rand32_get();
		blocksize = MIN(outlen, sizeof(ret));
		(void)memcpy((void *)udst, &ret, blocksize);
		udst += blocksize;
		outlen -= blocksize;
	}
}

#endif /* __GNUC__ */
