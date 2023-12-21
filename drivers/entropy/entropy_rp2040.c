/*
 * Copyright (c) 2023 Xudong Zheng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <pico/rand.h>
#include <zephyr/drivers/entropy.h>

#define DT_DRV_COMPAT raspberrypi_rp2040_rng

static int entropy_rp2040_get_entropy(const struct device *dev, uint8_t *buf, uint16_t len)
{
	__ASSERT_NO_MSG(buf != NULL);
	uint8_t *buf_bytes = (uint8_t *)buf;

	while (len > 0) {
		uint32_t word = get_rand_32();
		uint32_t to_copy = MIN(sizeof(word), len);

		memcpy(buf_bytes, &word, to_copy);
		buf_bytes += to_copy;
		len -= to_copy;
	}

	return 0;
}

static const struct entropy_driver_api entropy_rp2040_api_funcs = {
	.get_entropy = entropy_rp2040_get_entropy
};

static int entropy_rp2040_init(const struct device *dev)
{
	return 0;
}

DEVICE_DT_INST_DEFINE(0,
		    entropy_rp2040_init, NULL, NULL, NULL,
		    PRE_KERNEL_1, CONFIG_ENTROPY_INIT_PRIORITY,
		    &entropy_rp2040_api_funcs);
