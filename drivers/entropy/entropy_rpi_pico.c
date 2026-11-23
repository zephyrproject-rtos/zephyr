/*
 * Copyright (c) 2024 Xudong Zheng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <pico/rand.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/spinlock.h>

#define DT_DRV_COMPAT raspberrypi_pico_rng

static struct k_spinlock entropy_lock;

static int entropy_rpi_pico_get_entropy(const struct device *dev, uint8_t *buf, uint16_t len)
{
	__ASSERT_NO_MSG(buf != NULL);
	uint8_t *buf_bytes = buf;

	while (len > 0) {
		uint64_t value;
		size_t to_copy = MIN(sizeof(value), len);

		K_SPINLOCK(&entropy_lock) {
			value = get_rand_64();
		}
		memcpy(buf_bytes, &value, to_copy);
		buf_bytes += to_copy;
		len -= to_copy;
	}

	return 0;
}

static DEVICE_API(entropy, entropy_rpi_pico_api_funcs) = {
	.get_entropy = entropy_rpi_pico_get_entropy
};

DEVICE_DT_INST_DEFINE(0,
		    NULL, NULL, NULL, NULL,
		    PRE_KERNEL_1, CONFIG_ENTROPY_INIT_PRIORITY,
		    &entropy_rpi_pico_api_funcs);
