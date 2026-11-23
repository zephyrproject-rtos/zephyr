/*
 * Copyright (c) 2021 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT telink_b91_trng

#include <trng.h>
#include <zephyr/drivers/entropy.h>
#include <string.h>


/* API implementation: driver initialization */
static int entropy_b91_trng_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	trng_init();

	return 0;
}

/* API implementation: get_entropy */
static int entropy_b91_trng_get_entropy(const struct device *dev,
					uint8_t *buffer, uint16_t length)
{
	ARG_UNUSED(dev);

	uint32_t value = 0;

	while (length) {
		value = trng_rand();

		if (length >= sizeof(value)) {
			memcpy(buffer, &value, sizeof(value));
			buffer += sizeof(value);
			length -= sizeof(value);
		} else {
			memcpy(buffer, &value, length);
			break;
		}
	}

	return 0;
}

/* API implementation: get_entropy_isr */
static int entropy_b91_trng_get_entropy_isr(const struct device *dev,
					    uint8_t *buffer, uint16_t length,
					    uint32_t flags)
{
	ARG_UNUSED(flags);

	/* No specific handling in case of running from ISR, just call standard API */
	entropy_b91_trng_get_entropy(dev, buffer, length);

	return length;
}

/* Entropy driver APIs structure */
static DEVICE_API(entropy, entropy_b91_trng_api) = {
	.get_entropy = entropy_b91_trng_get_entropy,
	.get_entropy_isr = entropy_b91_trng_get_entropy_isr
};

/* Entropy driver registration */
DEVICE_DT_INST_DEFINE(0, entropy_b91_trng_init,
		      NULL, NULL, NULL,
		      PRE_KERNEL_1, CONFIG_ENTROPY_INIT_PRIORITY,
		      &entropy_b91_trng_api);
