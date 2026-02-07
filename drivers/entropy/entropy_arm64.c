/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#define DT_DRV_COMPAT zephyr_arm64_rng

static int entropy_arm64_get_entropy(const struct device *dev, uint8_t *buffer, uint16_t length)
{
	ARG_UNUSED(dev);

	int failure_counter = 0;

	while (length > 0) {
		unsigned long value;

		if (__builtin_aarch64_rndrrs(&value) != 0) {
			if (++failure_counter > CONFIG_ENTROPY_ARM64_RNG_MAX_RETRIES) {
				return -ENODATA;
			}
			k_msleep(CONFIG_ENTROPY_ARM64_RNG_RETRY_WAIT_MSEC);
		} else {
			size_t to_copy = MIN(length, sizeof(value));

			memcpy(buffer, &value, to_copy);
			buffer += to_copy;
			length -= to_copy;
			failure_counter = 0;
		}
	}

	return 0;
}

/* Entropy driver API */
static DEVICE_API(entropy, entropy_arm64_rng_api) = {
	.get_entropy = entropy_arm64_get_entropy,
};

/* Entropy driver registration */
DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, PRE_KERNEL_1, CONFIG_ENTROPY_INIT_PRIORITY,
		      &entropy_arm64_rng_api);
