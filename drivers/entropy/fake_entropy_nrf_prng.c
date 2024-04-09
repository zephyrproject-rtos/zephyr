/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <zephyr/drivers/entropy.h>

#define DT_DRV_COMPAT nordic_entropy_prng

/* This file implements a pseudo-RNG
 * https://vigna.di.unimi.it/xorshift/xoshiro128plus.c
 */

static uint32_t s[4];

static inline uint32_t rotl(const uint32_t x, int k)
{
	return (x << k) | (x >> (32 - k));
}

static uint32_t rng_next(void)
{
	const uint32_t result = rotl(s[0] + s[3], 7) + s[0];

	const uint32_t t = s[1] << 9;

	s[2] ^= s[0];
	s[3] ^= s[1];
	s[1] ^= s[2];
	s[0] ^= s[3];

	s[2] ^= t;

	s[3] = rotl(s[3], 11);

	return result;
}

static int entropy_prng_get_entropy(const struct device *dev, uint8_t *buffer, uint16_t length)
{
	ARG_UNUSED(dev);

	while (length) {
		/*
		 * Note that only 1 thread (Zephyr thread or HW models), runs at
		 * a time, therefore there is no need to use random_r()
		 */
		uint32_t value = rng_next();

		size_t to_copy = MIN(length, sizeof(long));

		memcpy(buffer, &value, to_copy);
		buffer += to_copy;
		length -= to_copy;
	}

	return 0;
}

static int entropy_prng_get_entropy_isr(const struct device *dev, uint8_t *buf, uint16_t len,
					uint32_t flags)
{
	ARG_UNUSED(flags);

	int err;

	/*
	 * entropy_prng_get_entropy() is also safe for ISRs
	 * and always produces data.
	 */
	err = entropy_prng_get_entropy(dev, buf, len);
	if (err < 0) {
		return err;
	} else {
		return len;
	}
}

static int entropy_prng_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* Picked some arbitrary initial seed. */
	s[0] = 0xAF568BC0;
	s[1] = 0xAC34307E;
	s[2] = 0x9B7F6DD1;
	s[3] = 0xD84319FC;
	return 0;
}

static const struct entropy_driver_api entropy_prng_api_funcs = {
	.get_entropy = entropy_prng_get_entropy, .get_entropy_isr = entropy_prng_get_entropy_isr};

DEVICE_DT_INST_DEFINE(0, entropy_prng_init, NULL, NULL, NULL, PRE_KERNEL_1,
		      CONFIG_ENTROPY_INIT_PRIORITY, &entropy_prng_api_funcs);
