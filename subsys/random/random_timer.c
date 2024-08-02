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

#include <zephyr/random/random.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <string.h>

#if defined(__GNUC__)

static struct k_spinlock rand32_lock;

/**
 * @brief Get a 32 bit random number
 *
 * This pseudo-random number generator returns values that are based off the
 * target's clock counter, which means that successive calls will return
 * different values.
 *
 * @return a 32-bit number
 */
static inline uint32_t rand32_get(void)
{
	/* initial seed value */
	static uint64_t state = (uint64_t)CONFIG_TIMER_RANDOM_INITIAL_STATE;
	k_spinlock_key_t key = k_spin_lock(&rand32_lock);

	state = state + k_cycle_get_32();
	state = state * 2862933555777941757ULL + 3037000493ULL;
	uint32_t val = (uint32_t)(state >> 32);

	k_spin_unlock(&rand32_lock, key);
	return val;
}

/**
 * @brief Fill destination buffer with random numbers
 *
 * The pseudo-random number generator returns values that are based off the
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

	while (outlen > 0) {
		ret = rand32_get();
		blocksize = MIN(outlen, sizeof(ret));
		(void)memcpy((void *)udst, &ret, blocksize);
		udst += blocksize;
		outlen -= blocksize;
	}
}
#endif /* __GNUC__ */
