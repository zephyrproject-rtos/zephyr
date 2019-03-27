/*
 * Copyright (c) 2019 ML!PA Consulting GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <entropy.h>
#include <errno.h>
#include <init.h>
#include <soc.h>
#include <string.h>

struct trng_sam0_dev_cfg {
	Trng *regs;
};

#define DEV_CFG(dev) \
	((const struct trng_sam0_dev_cfg *const)(dev)->config->config_info)

static int entropy_sam0_wait_ready(Trng *const trng)
{
	/* According to the reference manual, the generator provides
	 * one 32-bit random value every 84 peripheral clock cycles.
	 * MCK may not be smaller than HCLK/4, so it should not take
	 * more than 336 HCLK ticks. Assuming the CPU can do 1
	 * instruction per HCLK the number of times to loop before
	 * the TRNG is ready is less than 1000. And that is when
	 * assuming the loop only takes 1 instruction. So looping a
	 * million times should be more than enough.
	 */
	int timeout = 1000000;

	while (!trng->INTFLAG.bit.DATARDY) {
		if (timeout-- == 0) {
			return -ETIMEDOUT;
		}

		k_yield();
	}

	return 0;
}

static int entropy_sam0_get_entropy(struct device *dev, u8_t *buffer,
				   u16_t length)
{
	Trng *const trng = DEV_CFG(dev)->regs;

	while (length > 0) {
		size_t to_copy;
		u32_t value;
		int res;

		res = entropy_sam0_wait_ready(trng);
		if (res < 0) {
			return res;
		}

		value = TRNG->DATA.reg;
		to_copy = MIN(length, sizeof(value));

		memcpy(buffer, &value, to_copy);
		buffer += to_copy;
		length -= to_copy;
	}

	return 0;
}

static int entropy_sam0_init(struct device *dev)
{
	Trng *const trng = DEV_CFG(dev)->regs;

	/* Enable the MCLK */
	MCLK->APBCMASK.bit.TRNG_ = 1;

	/* Enable the TRNG */
	trng->CTRLA.bit.ENABLE = 1;

	return 0;
}

static const struct entropy_driver_api entropy_sam0_api = {
	.get_entropy = entropy_sam0_get_entropy
};

static const struct trng_sam0_dev_cfg trng_sam0_cfg = {
	.regs = (Trng *)DT_ATMEL_SAM0_TRNG_0_BASE_ADDRESS,
};

DEVICE_AND_API_INIT(entropy_sam0, CONFIG_ENTROPY_NAME,
		    entropy_sam0_init, NULL, &trng_sam0_cfg,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &entropy_sam0_api);
