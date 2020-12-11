/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_trng

#include <string.h>
#include <drivers/entropy.h>

static inline uint32_t entropy_esp32_get_u32(void)
{
	/*
	 * APB Address:   0x60035144 (Safe,slower writes)
	 * DPORT Address: 0x3ff75144 (write bug, fast writes)
	 * In this case it won't make a difference because it is read only
	 * More info available at:
	 * https://www.esp32.com/viewtopic.php?f=2&t=3033&p=14227
	 * also check: ECO and Workarounds for Bugs Document, point 3.3
	 */
	volatile uint32_t *rng_data_reg = (uint32_t *)DT_INST_REG_ADDR(0);

	/* Read just once.  This is not optimal as the generator has
	 * limited throughput due to scarce sources of entropy, specially
	 * with the radios turned off.  Might want to revisit this.
	 */
	return *rng_data_reg;
}

static int entropy_esp32_get_entropy(const struct device *device,
				     uint8_t *buf, uint16_t len)
{
	while (len) {
		uint32_t v = entropy_esp32_get_u32();

		if (len >= sizeof(v)) {
			memcpy(buf, &v, sizeof(v));

			buf += sizeof(v);
			len -= sizeof(v);
		} else {
			memcpy(buf, &v, len);
			break;
		}
	}

	return 0;
}

static int entropy_esp32_init(const struct device *device)
{
	return 0;
}

static struct entropy_driver_api entropy_esp32_api_funcs = {
	.get_entropy = entropy_esp32_get_entropy
};

DEVICE_AND_API_INIT(entropy_esp32, DT_INST_LABEL(0),
		    entropy_esp32_init, NULL, NULL,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &entropy_esp32_api_funcs);
