/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_kinetis_trng

#include <device.h>
#include <drivers/entropy.h>
#include <random/rand32.h>
#include <init.h>

#include "fsl_trng.h"

struct mcux_entropy_config {
	TRNG_Type *base;
};

static int entropy_mcux_trng_get_entropy(const struct device *dev,
					 uint8_t *buffer,
					 uint16_t length)
{
	const struct mcux_entropy_config *config = dev->config;
	status_t status;

	ARG_UNUSED(dev);

	status = TRNG_GetRandomData(config->base, buffer, length);
	__ASSERT_NO_MSG(!status);

	return 0;
}

static const struct entropy_driver_api entropy_mcux_trng_api_funcs = {
	.get_entropy = entropy_mcux_trng_get_entropy
};

static struct mcux_entropy_config entropy_mcux_config = {
	.base = (TRNG_Type *)DT_INST_REG_ADDR(0)
};

static int entropy_mcux_trng_init(const struct device *);

DEVICE_AND_API_INIT(entropy_mcux_trng, DT_INST_LABEL(0),
		    entropy_mcux_trng_init, NULL, &entropy_mcux_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &entropy_mcux_trng_api_funcs);

static int entropy_mcux_trng_init(const struct device *dev)
{
	const struct mcux_entropy_config *config = dev->config;
	trng_config_t conf;
	status_t status;

	ARG_UNUSED(dev);

	status = TRNG_GetDefaultConfig(&conf);
	__ASSERT_NO_MSG(!status);

	status = TRNG_Init(config->base, &conf);
	__ASSERT_NO_MSG(!status);

	return 0;
}
