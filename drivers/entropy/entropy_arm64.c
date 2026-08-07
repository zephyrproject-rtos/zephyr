/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

static int entropy_arm64_get_entropy_isr(const struct device *dev, uint8_t *buffer, uint16_t length,
					 uint32_t flags)
{
	ARG_UNUSED(dev);

	int max_retries =
		(flags & ENTROPY_BUSYWAIT) == 0 ? 0 : CONFIG_ENTROPY_ARM64_RNG_MAX_RETRIES;
	int failure_counter = 0;
	uint16_t generated_bytes = 0;

	while (length > 0) {
		unsigned long value;

		if (__builtin_aarch64_rndrrs(&value) != 0) {
			if (++failure_counter > max_retries) {
				break;
			}
			k_busy_wait(CONFIG_ENTROPY_ARM64_RNG_RETRY_WAIT_USEC);
		} else {
			size_t to_copy = MIN(length, sizeof(value));

			memcpy(buffer, &value, to_copy);
			buffer += to_copy;
			length -= to_copy;
			generated_bytes += to_copy;
			failure_counter = 0;
		}
	}

	return (int)generated_bytes;
}

static int entropy_arm64_get_entropy(const struct device *dev, uint8_t *buffer, uint16_t length)
{
	uint16_t generated_bytes =
		(uint16_t)entropy_arm64_get_entropy_isr(dev, buffer, length, ENTROPY_BUSYWAIT);

	return generated_bytes == length ? 0 : -ENODATA;
}

/* Entropy driver API */
static DEVICE_API(entropy, entropy_arm64_rng_api) = {
	.get_entropy = entropy_arm64_get_entropy,
	.get_entropy_isr = entropy_arm64_get_entropy_isr,
};

DEVICE_DEFINE(arm64_rng_entropy_device, "ARM64_RNG_ENTROPY", NULL, NULL, NULL, NULL, PRE_KERNEL_1,
	      CONFIG_ENTROPY_INIT_PRIORITY, &entropy_arm64_rng_api);

const struct device *const z_arch_entropy_dev = DEVICE_GET(arm64_rng_entropy_device);
