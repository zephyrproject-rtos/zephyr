/*
 * Copyright(c) 2026, Realtek Semiconductor Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT realtek_bee_trng

#include <zephyr/drivers/entropy.h>
#include <string.h>

#include <utils.h>

static int entropy_bee_get_entropy(const struct device *dev, uint8_t *buffer, uint16_t length)
{
	ARG_UNUSED(dev);

	while (length > 0) {
		size_t to_copy;
		uint32_t value;

		value = platform_random(0xFFFFFFFFU);

		if (length % 4 == 0) {
			to_copy = 4;
		} else {
			to_copy = length % 4;
		}

		memcpy(buffer, &value, to_copy);
		buffer += to_copy;
		length -= to_copy;
	}

	return 0;
}

static DEVICE_API(entropy, entropy_bee_trng_api_funcs) = {
	.get_entropy = entropy_bee_get_entropy,
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, PRE_KERNEL_1, CONFIG_ENTROPY_INIT_PRIORITY,
		      &entropy_bee_trng_api_funcs);
