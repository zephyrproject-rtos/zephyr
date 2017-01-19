/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include "util.h"

uint8_t util_ones_count_get(uint8_t *octets, uint8_t octets_len)
{
	uint8_t one_count = 0;

	while (octets_len--) {
		uint8_t bite;

		bite = *octets;
		while (bite) {
			bite &= (bite - 1);
			one_count++;
		}
		octets++;
	}

	return one_count;
}
