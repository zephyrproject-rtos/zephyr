/*
 * Copyright 2022, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_caam

#include <zephyr/device.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/random/random.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>

#include "fsl_caam.h"

struct mcux_entropy_config {
	CAAM_Type *base;
};

static caam_job_ring_interface_t jrif0 __attribute__((__section__(".nocache")));
static uint8_t rng_buff_pool[CONFIG_ENTRY_MCUX_CAAM_POOL_SIZE]
					__attribute__((__section__(".nocache")));


/* This semaphore is needed to prevent race condition to static variables in the HAL driver */
K_SEM_DEFINE(mcux_caam_sem, 1, 1)

static int entropy_mcux_caam_get_entropy(const struct device *dev,
					 uint8_t *buffer,
					 uint16_t length)
{
	const struct mcux_entropy_config *config = dev->config;
	status_t status;
	caam_handle_t handle;
	uint16_t read_length = 0;
	uint16_t insert_idx = 0;
	int ret = 0;
	k_timeout_t sem_timeout = K_MSEC(10);

	handle.jobRing = kCAAM_JobRing0;

	/*
	 * The buffer passed to the CAAM RNG function needs to be in non-cache
	 * memory. Use an intermediate buffer to stage the data to the user
	 * buffer.
	 */
	while (insert_idx < length) {
		read_length = MIN(sizeof(rng_buff_pool), (length - insert_idx));

		ret = k_sem_take(&mcux_caam_sem, sem_timeout);
		if (ret) {
			return ret;
		}

		status = CAAM_RNG_GetRandomData(
				config->base, &handle, kCAAM_RngStateHandle0,
				&rng_buff_pool[0], read_length, kCAAM_RngDataAny, NULL);

		k_sem_give(&mcux_caam_sem);

		memcpy(&buffer[insert_idx], &rng_buff_pool[0], read_length);
		insert_idx += read_length;
	}

	return 0;
}

static DEVICE_API(entropy, entropy_mcux_caam_api_funcs) = {
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
	conf.jobRingInterface[0] = &jrif0;

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
