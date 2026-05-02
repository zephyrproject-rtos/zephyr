/*
 * Copyright (c) 1991, Daniel J. Bernstein
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This software is in the PD as of approximately 2007. The story
 * behind it (and other DJB software) is quite amazing. Thanks Dan!!
 *
 * Many distributors have relicensed it under e.g. BSD, MIT, and others.
 *
 * It is not clear what the original license name is or how to declare it
 * using SPDX terms, since it explicitly has no license. I think it makes
 * sense to use the default Zephyr licensing.
 *
 * Note: this is not a cryptographically strong hash algorithm.
 *
 * For details, please see
 * https://cr.yp.to/rights.html
 * https://cr.yp.to/distributors.html
 * http://thedjbway.b0llix.net/license_free.html
 *
 * https://groups.google.com/g/comp.lang.c/c/lSKWXiuNOAk
 * https://theartincode.stanis.me/008-djb2/
 * http://www.cse.yorku.ca/~oz/hash.html
 * https://bit.ly/3IxUVvC
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/sys/hash_function.h>

uint32_t sys_hash32_djb2(const void *str, size_t n)
{
	uint32_t hash;
	const uint8_t *d;

	/* The number 5381 is the initializer for the djb2 hash */
	for (hash = 5381, d = str; n > 0; --n, ++d) {
		/* The djb2 hash multiplier is 33 (i.e. 2^5 + 1) */
		hash = (hash << 5) + hash;
		hash ^= *d;
	}

	return hash;
}
