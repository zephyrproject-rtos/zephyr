/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <random.h>

static inline u32_t random_esp32_get_u32(void)
{
	/* The documentation specifies the random number generator at the
	 * following address, which is at odds with the SDK, that specifies
	 * it at 0x60035144.  The fact that they're 0x200c0000 bytes apart
	 * (lower 16 bits are the same) suggests this might be the same
	 * register, just mirrored somewhere else in the address space.
	 * Confirmation is required.
	 */
	volatile u32_t *rng_data_reg = (u32_t *)0x3ff75144;

	/* Read just once.  This is not optimal as the generator has
	 * limited throughput due to scarce sources of entropy, specially
	 * with the radios turned off.  Might want to revisit this.
	 */
	return *rng_data_reg;
}

static int random_esp32_get_entropy(struct device *device, u8_t *buf, u16_t len)
{
	while (len) {
		u32_t v = random_esp32_get_u32();

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

static int random_esp32_init(struct device *device)
{
	return 0;
}

static struct random_driver_api random_esp32_api_funcs = {
	.get_entropy = random_esp32_get_entropy
};

DEVICE_AND_API_INIT(random_esp32, CONFIG_RANDOM_NAME,
		    random_esp32_init, NULL, NULL,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &random_esp32_api_funcs);

u32_t sys_rand32_get(void)
{
	return random_esp32_get_u32();
}
