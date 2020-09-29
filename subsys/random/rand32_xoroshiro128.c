/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: CC0-1.0
 *
 * Based on code written in 2016 by David Blackman and Sebastiano Vigna
 * (vigna@acm.org)
 *
 * To the extent possible under law, the author has dedicated all copyright
 * and related and neighboring rights to this software to the public domain
 * worldwide. This software is distributed without any warranty.
 *
 * See <http://creativecommons.org/publicdomain/zero/1.0/>.
 */

/* This is the successor to xorshift128+. It is the fastest full-period
 * generator passing BigCrush without systematic failures, but due to the
 * relatively short period it is acceptable only for applications with a
 * mild amount of parallelism; otherwise, use a xorshift1024* generator.
 *
 * Beside passing BigCrush, this generator passes the PractRand test suite
 * up to (and included) 16TB, with the exception of binary rank tests, as
 * the lowest bit of this generator is an LSFR. The next bit is not an
 * LFSR, but in the long run it will fail binary rank tests, too. The
 * other bits have no LFSR artifacts.
 *
 * We suggest to use a sign test to extract a random Boolean value, and
 * right shifts to extract subsets of bits.
 *
 * Note that the generator uses a simulated rotate operation, which most C
 * compilers will turn into a single instruction. In Java, you can use
 * Long.rotateLeft(). In languages that do not make low-level rotation
 * instructions accessible xorshift128+ could be faster.
 *
 * The state must be seeded so that it is not everywhere zero. If you have
 * a 64-bit seed, we suggest to seed a splitmix64 generator and use its
 * output to fill s.
 */

#include <init.h>
#include <device.h>
#include <drivers/entropy.h>
#include <kernel.h>
#include <string.h>

static uint64_t state[2];

static inline uint64_t rotl(const uint64_t x, int k)
{
	return (x << k) | (x >> (64 - k));
}

static int xoroshiro128_initialize(const struct device *dev)
{
	dev = device_get_binding(DT_CHOSEN_ZEPHYR_ENTROPY_LABEL);
	if (!dev) {
		return -EINVAL;
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

static uint32_t xoroshiro128_next(void)
{
	const uint64_t s0 = state[0];
	uint64_t s1 = state[1];
	const uint64_t result = s0 + s1;

	s1 ^= s0;
	state[0] = rotl(s0, 55) ^ s1 ^ (s1 << 14);
	state[1] = rotl(s1, 36);

	return (uint32_t)result;
}

uint32_t z_impl_sys_rand32_get(void)
{
	uint32_t ret;

	ret = xoroshiro128_next();

	return ret;
}

void z_impl_sys_rand_get(void *dst, size_t outlen)
{
	uint32_t ret;
	uint32_t blocksize = 4;
	uint32_t len = 0;
	uint32_t *udst = (uint32_t *)dst;

	while (len < outlen) {
		ret = xoroshiro128_next();
		if ((outlen-len) < sizeof(ret)) {
			blocksize = len;
			(void)memcpy(udst, &ret, blocksize);
		} else {
			(*udst++) = ret;
		}
		len += blocksize;
	}
}

/* In-tree entropy drivers will initialize in PRE_KERNEL_1; ensure that they're
 * initialized properly before initializing ourselves.
 */
SYS_INIT(xoroshiro128_initialize, PRE_KERNEL_2,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
