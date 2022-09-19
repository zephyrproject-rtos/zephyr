/*
 * Copyright 2022, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_caam

#include <zephyr/device.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/random/rand32.h>
#include <zephyr/init.h>

#include "fsl_caam.h"

struct mcux_entropy_config {
	CAAM_Type *base;
};

static caam_job_ring_interface_t jrif;

static int entropy_mcux_caam_get_entropy(const struct device *dev,
					 uint8_t *buffer,
					 uint16_t length)
{
	const struct mcux_entropy_config *config = dev->config;
	status_t status;
	caam_handle_t handle;

	handle.jobRing = kCAAM_JobRing0;

	status = CAAM_RNG_GetRandomData(
			config->base, &handle, kCAAM_RngStateHandle0,
			buffer, length, kCAAM_RngDataAny, NULL);
	__ASSERT_NO_MSG(!status);

	return 0;
}

static const struct entropy_driver_api entropy_mcux_caam_api_funcs = {
	.get_entropy = entropy_mcux_caam_get_entropy
};

static const struct mcux_entropy_config entropy_mcux_config = {
	.base = (CAAM_Type *)DT_INST_REG_ADDR(0)
};

static int entropy_mcux_caam_init(const struct device *dev)
{
	const struct mcux_entropy_config *config = dev->config;
	caam_config_t conf;
	status_t status;

	CAAM_GetDefaultConfig(&conf);
	conf.jobRingInterface[0] = &jrif;

	status = CAAM_Init(config->base, &conf);
	__ASSERT_NO_MSG(!status);

	if (status != 0) {
		return -ENODEV;
	}

	return 0;
}

DEVICE_DT_INST_DEFINE(0,
		    entropy_mcux_caam_init, NULL, NULL,
		    &entropy_mcux_config,
		    PRE_KERNEL_1, CONFIG_ENTROPY_INIT_PRIORITY,
		    &entropy_mcux_caam_api_funcs);
