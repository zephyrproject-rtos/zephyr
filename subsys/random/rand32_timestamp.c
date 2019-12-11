/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Non-random number generator based on x86 CPU timestamp
 *
 * This module provides a non-random implementation of sys_rand32_get(), which
 * is not meant to be used in a final product as a truly random number
 * generator. It was provided to allow testing on a platform that does not (yet)
 * provide a random number generator.
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <random/rand32.h>
#include <string.h>

/**
 *
 * @brief Get a 32 bit random number
 *
 * The non-random number generator returns values that are based off the
 * CPU's timestamp counter, which means that successive calls will normally
 * display ever-increasing values.
 *
 * @return a 32-bit number
 */

u32_t sys_rand32_get(void)
{
	return z_do_read_cpu_timestamp32();
}

/**
 *
 * @brief Fill destination buffer with random numbers
 *
 * The non-random number generator returns values that are based off the
 * target's clock counter, which means that successive calls will return
 * different values.
 *
 * @param dst destination buffer to fill
 * @param outlen size of destination buffer to fill
 *
 * @return N/A
 */

void sys_rand_get(void *dst, size_t outlen)
{
	u32_t len = 0;
	u32_t blocksize = 4;
	u32_t ret;
	u32_t *udst = (u32_t *)dst;

	while (len < outlen) {
		ret = sys_rand32_get();
		if ((outlen-len) < sizeof(ret)) {
			blocksize = len;
			(void *)memcpy(udst, &ret, blocksize);
		} else {
			(*udst++) = ret;
		}
		len += blocksize;
	}
}
