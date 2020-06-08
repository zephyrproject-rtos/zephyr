/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <drivers/entropy.h>
#include "util.h"

/**
 * @brief Population count: Count the number of bits set to 1
 * @details
 * TODO: Faster methods available at [1].
 * [1] http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
 *
 * @param octets     Data to count over
 * @param octets_len Must not be bigger than 255/8 = 31 bytes
 *
 * @return popcnt of 'octets'
 */
u8_t util_ones_count_get(u8_t *octets, u8_t octets_len)
{
	u8_t one_count = 0U;

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

int util_rand(void *buf, size_t len)
{
	static struct device *dev;

	if (unlikely(!dev)) {
		/* Only one entropy device exists, so this is safe even
		 * if the whole operation isn't atomic.
		 */
		dev = device_get_binding(DT_CHOSEN_ZEPHYR_ENTROPY_LABEL);
		__ASSERT((dev != NULL),
			"Device driver for %s (DT_CHOSEN_ZEPHYR_ENTROPY_LABEL) not found. "
			"Check your build configuration!",
			DT_CHOSEN_ZEPHYR_ENTROPY_LABEL);
	}

	return entropy_get_entropy(dev, (u8_t *)buf, len);
}
