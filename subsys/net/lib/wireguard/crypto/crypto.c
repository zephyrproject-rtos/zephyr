/*
 * Copyright (c) 2021 Daniel Hope (www.floorsense.nz)
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "crypto.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

void crypto_zero(void *dest, size_t len)
{
	volatile uint8_t *p = (uint8_t *)dest;

	while (len--) {
		*p++ = 0;
	}
}

bool crypto_equal(const void *pa, const void *pb, size_t size)
{
	uint8_t neq = 0;
	const uint8_t *a = pa;
	const uint8_t *b = pb;

	while (size > 0) {
		neq |= *(uint8_t *)a ^ *(uint8_t *)b;
		a += 1;
		b += 1;
		size -= 1;
	}
	return (neq) ? false : true;
}
