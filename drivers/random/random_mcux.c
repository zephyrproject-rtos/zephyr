/*
 * Copyright (c) 2016 ARM Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <random.h>
#include <drivers/rand32.h>
#include <init.h>

#include "fsl_rnga.h"

static uint8_t random_mcux_get_uint8(void)
{
	uint32_t random;
	uint8_t output = 0;
	int i;

	RNGA_SetMode(RNG, kRNGA_ModeNormal);
	/* The Reference manual states that back to back reads from
	 * the RNGA deliver one or two bits of entropy per 32-bit
	 * word, therefore to deliver 8 bits of entropy we need
	 * between 4 and 8 samples.  Conservatively, we take 8.
	 */
	for (i = 0; i < 8; i++) {
		status_t status;

		status = RNGA_GetRandomData(RNG, &random, sizeof(random));
		__ASSERT_NO_MSG(!status);
		output ^= random;
	}

	RNGA_SetMode(RNG, kRNGA_ModeSleep);

	return output;
}

static int random_mcux_get_entropy(struct device *dev, uint8_t *buffer,
				   uint16_t length)
{
	int i;

	ARG_UNUSED(dev);

	for (i = 0; i < length; i++) {
		buffer[i] = random_mcux_get_uint8();
	}

	return 0;
}

static const struct random_driver_api random_mcux_api_funcs = {
	.get_entropy = random_mcux_get_entropy
};

static int random_mcux_init(struct device *);

DEVICE_AND_API_INIT(random_mcux, CONFIG_RANDOM_NAME,
		    random_mcux_init, NULL, NULL,
		    PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &random_mcux_api_funcs);

static int random_mcux_init(struct device *dev)
{
	uint32_t seed = k_cycle_get_32();

	ARG_UNUSED(dev);

	RNGA_Init(RNG);

	/* The range of seed values acquired by this method is likely
	 * to be relatively small.  The RNGA hardware uses two free
	 * running oscillators to add entropy to the seed value, we
	 * take care below to ensure the read rate is lower than the
	 * rate at which the hardware can add entropy.
	 */
	RNGA_Seed(RNG, seed);
	RNGA_SetMode(RNG, kRNGA_ModeSleep);
	return 0;
}

uint32_t sys_rand32_get(void)
{
	uint32_t output;
	int r;

	r = random_mcux_get_entropy(DEVICE_GET(random_mcux),
				    (uint8_t *) &output, sizeof(output));
	__ASSERT_NO_MSG(!r);

	return output;
}
