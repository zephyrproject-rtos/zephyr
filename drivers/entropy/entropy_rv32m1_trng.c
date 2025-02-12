/*
 * Copyright 2019 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT openisa_rv32m1_trng

#include <zephyr/device.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/random/random.h>
#include <zephyr/init.h>

#include "fsl_trng.h"

struct rv32m1_entropy_config {
	TRNG_Type *base;
};

static int entropy_rv32m1_trng_get_entropy(const struct device *dev,
					   uint8_t *buffer,
					   uint16_t length)
{
	const struct rv32m1_entropy_config *config = dev->config;
	status_t status;

	ARG_UNUSED(dev);

	status = TRNG_GetRandomData(config->base, buffer, length);
	__ASSERT_NO_MSG(!status);

	return 0;
}

static const struct entropy_driver_api entropy_rv32m1_trng_api_funcs = {
	.get_entropy = entropy_rv32m1_trng_get_entropy
};

static struct rv32m1_entropy_config entropy_rv32m1_config = {
	.base = (TRNG_Type *)DT_INST_REG_ADDR(0)
};

static int entropy_rv32m1_trng_init(const struct device *);

DEVICE_DT_INST_DEFINE(0,
		    entropy_rv32m1_trng_init, NULL,
		    NULL, &entropy_rv32m1_config,
		    PRE_KERNEL_1, CONFIG_ENTROPY_INIT_PRIORITY,
		    &entropy_rv32m1_trng_api_funcs);

static int entropy_rv32m1_trng_init(const struct device *dev)
{
	const struct rv32m1_entropy_config *config = dev->config;
	trng_config_t conf;
	status_t status;

	ARG_UNUSED(dev);

	status = TRNG_GetDefaultConfig(&conf);
	__ASSERT_NO_MSG(!status);

	status = TRNG_Init(config->base, &conf);
	__ASSERT_NO_MSG(!status);

	return 0;
}
