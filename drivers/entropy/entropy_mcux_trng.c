/*
 * Copyright (c) 2017 Linaro Limited.
 * Copyright 2025 NXP
 * Copyright (c) 2026 Xsight Labs Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_kinetis_trng

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/random/random.h>
#include <zephyr/pm/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/util.h>

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

	/*
	 * TRNG_GetRandomData() may write random data in 32-bit word
	 * granularity into the destination buffer. When the SDK is built
	 * with TRNG_SW_HEALTH_TESTS enabled it always copies full words,
	 * which would overflow a caller buffer whose length is not a
	 * multiple of 4 bytes. Use a word-aligned bounce buffer and copy
	 * only the requested number of bytes to guarantee we never write
	 * past buffer[length - 1].
	 */
	while (length > 0U) {
		uint32_t word;
		uint16_t chunk = MIN(length, (uint16_t)sizeof(word));

		status = TRNG_GetRandomData(config->base, &word, sizeof(word));
		if (status != kStatus_Success) {
			return -EIO;
		}

		memcpy(buffer, &word, chunk);
		buffer += chunk;
		length -= chunk;
	}

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
	if (status != kStatus_Success) {
		return -EIO;
	}

	status = TRNG_Init(config->base, &conf);
	if (status != kStatus_Success) {
		return -EIO;
	}

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
		return entropy_mcux_trng_init(dev);
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
