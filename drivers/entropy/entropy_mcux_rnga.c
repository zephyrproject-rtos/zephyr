/*
 * Copyright (c) 2016 ARM Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_kinetis_rnga

#include <device.h>
#include <drivers/entropy.h>
#include <random/rand32.h>
#include <init.h>

#include "fsl_rnga.h"

static uint8_t entropy_mcux_rnga_get_uint8(void)
{
	uint32_t random;
	uint8_t output = 0U;
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

static int entropy_mcux_rnga_get_entropy(const struct device *dev,
					 uint8_t *buffer,
					 uint16_t length)
{
	int i;

	ARG_UNUSED(dev);

	for (i = 0; i < length; i++) {
		buffer[i] = entropy_mcux_rnga_get_uint8();
	}

	return 0;
}

static const struct entropy_driver_api entropy_mcux_rnga_api_funcs = {
	.get_entropy = entropy_mcux_rnga_get_entropy
};

static int entropy_mcux_rnga_init(const struct device *);

DEVICE_AND_API_INIT(entropy_mcux_rnga, DT_INST_LABEL(0),
		    entropy_mcux_rnga_init, NULL, NULL,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &entropy_mcux_rnga_api_funcs);

static int entropy_mcux_rnga_init(const struct device *dev)
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
