/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include "util.h"

u8_t util_ones_count_get(u8_t *octets, u8_t octets_len)
{
	u8_t one_count = 0;

	while (octets_len--) {
		u8_t bite;

		bite = *octets;
		while (bite) {
			bite &= (bite - 1);
			one_count++;
		}
		octets++;
	}

	return one_count;
}
