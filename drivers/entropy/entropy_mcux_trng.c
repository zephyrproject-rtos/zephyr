/*
 * Copyright (c) 2017 Linaro Limited.
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_kinetis_trng

#include <zephyr/device.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/random/random.h>
#include <zephyr/pm/device.h>
#include <zephyr/init.h>

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

	status = TRNG_GetRandomData(config->base, buffer, length);
	__ASSERT_NO_MSG(!status);

	return 0;
}

static DEVICE_API(entropy, entropy_mcux_trng_api_funcs) = {
	.get_entropy = entropy_mcux_trng_get_entropy
};

static struct mcux_entropy_config entropy_mcux_config = {
	.base = (TRNG_Type *)DT_INST_REG_ADDR(0)
};

static int entropy_mcux_trng_init(const struct device *dev)
{
	const struct mcux_entropy_config *config = dev->config;
	trng_config_t conf;
	status_t status;

	status = TRNG_GetDefaultConfig(&conf);
	__ASSERT_NO_MSG(!status);

	status = TRNG_Init(config->base, &conf);
	__ASSERT_NO_MSG(!status);

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int entropy_mcux_trng_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
		break;
	case PM_DEVICE_ACTION_TURN_ON:
		entropy_mcux_trng_init(dev);
		break;
	default:
		return -ENOTSUP;
	}
	return 0;
}
#endif /*CONFIG_PM_DEVICE*/

PM_DEVICE_DT_INST_DEFINE(0, entropy_mcux_trng_pm_action);
DEVICE_DT_INST_DEFINE(0,
		    entropy_mcux_trng_init, PM_DEVICE_DT_INST_GET(0), NULL,
		    &entropy_mcux_config,
		    PRE_KERNEL_1, CONFIG_ENTROPY_INIT_PRIORITY,
		    &entropy_mcux_trng_api_funcs);
