/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpc_rng

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/random/random.h>
#include <zephyr/init.h>

#include "fsl_rng.h"

/*
 * On LPC54xxx the generator is reached through the boot ROM, so the HAL
 * component takes no peripheral base and has no init function. The driver
 * only enables the RNG clock; the SoC init powers the RNG up.
 */
#if defined(CONFIG_SOC_SERIES_LPC54XXX)
#define ENTROPY_MCUX_RNG_ROM 1
#else
#define ENTROPY_MCUX_RNG_ROM 0
#endif

struct mcux_entropy_config {
#if ENTROPY_MCUX_RNG_ROM
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
#else
	RNG_Type *base;
#endif
};

static int entropy_mcux_rng_get_entropy(const struct device *dev,
					 uint8_t *buffer,
					 uint16_t length)
{
	status_t status;
	int ret;

#if ENTROPY_MCUX_RNG_ROM
	ARG_UNUSED(dev);

	status = RNG_GetRandomData(buffer, length);
#else
	const struct mcux_entropy_config *config = dev->config;

	status = RNG_GetRandomData(config->base, buffer, length);
#endif

	/* It seems that this function returns either success or
	 * invalid argument.
	 */
	switch (status) {
	case kStatus_InvalidArgument:
		ret = -EINVAL;
		break;
	case kStatus_Success:
		ret = 0;
		break;
	default:
		ret = -ENODATA;
		break;
	}

	return ret;
}

static DEVICE_API(entropy, entropy_mcux_rng_api_funcs) = {
	.get_entropy = entropy_mcux_rng_get_entropy
};

#if ENTROPY_MCUX_RNG_ROM
static const struct mcux_entropy_config entropy_mcux_config = {
	.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
	.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(0, name),
};

static int entropy_mcux_rng_init(const struct device *dev)
{
	const struct mcux_entropy_config *config = dev->config;

	if (!device_is_ready(config->clock_dev)) {
		return -ENODEV;
	}

	return clock_control_on(config->clock_dev, config->clock_subsys);
}
#else
static const struct mcux_entropy_config entropy_mcux_config = {
	.base = (RNG_Type *)DT_INST_REG_ADDR(0)
};

static int entropy_mcux_rng_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	RNG_Init(entropy_mcux_config.base);

	return 0;
}
#endif

DEVICE_DT_INST_DEFINE(0,
		    entropy_mcux_rng_init, NULL, NULL,
		    &entropy_mcux_config,
		    PRE_KERNEL_1, CONFIG_ENTROPY_INIT_PRIORITY,
		    &entropy_mcux_rng_api_funcs);
