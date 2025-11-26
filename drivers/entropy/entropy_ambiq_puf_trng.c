/*
 * Copyright (c) 2025 Ambiq Micro, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ambiq_puf_trng

#include <zephyr/logging/log.h>
#include <zephyr/drivers/entropy.h>

/* ambiq-sdk includes */
#include <soc.h>

LOG_MODULE_REGISTER(ambiq_puf_trng_entropy, CONFIG_ENTROPY_LOG_LEVEL);

static int entropy_ambiq_get_trng(const struct device *dev, uint8_t *buffer, uint16_t length)
{
	ARG_UNUSED(dev);
	uint32_t status;

	/* Validate input parameters */
	if (length == 0 || buffer == NULL) {
		return -EINVAL;
	}

	/* Use the HAL function to get entropy */
	status = am_hal_puf_get_entropy(buffer, length);
	if (status != AM_HAL_STATUS_SUCCESS) {
		LOG_ERR("Failed to get entropy, error: 0x%x", status);
		return -EIO;
	}

	return 0;
}

static int entropy_ambiq_get_trng_isr(const struct device *dev, uint8_t *buffer,
				      uint16_t length, uint32_t flags)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(flags);
	uint32_t status;

	/* Validate input parameters */
	if (length == 0 || buffer == NULL) {
		return -EINVAL;
	}

	/* Use the HAL function to get entropy - same as non-ISR version
	 * since the HAL function is already non-blocking and safe to call from ISR
	 */
	status = am_hal_puf_get_entropy(buffer, length);
	if (status != AM_HAL_STATUS_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static int entropy_ambiq_trng_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	uint32_t status;

	/* Initialize the PUF entropy peripheral (powers on OTP if needed) */
	status = am_hal_puf_entropy_init();
	if (status != AM_HAL_STATUS_SUCCESS) {
		LOG_ERR("Failed to initialize PUF entropy, error: 0x%x", status);
		return -EBUSY;
	}

	return 0;
}

static int entropy_ambiq_trng_deinit(const struct device *dev)
{
	ARG_UNUSED(dev);
	uint32_t status;

	/* Deinitialize the PUF entropy peripheral (powers down OTP if we enabled it) */
	status = am_hal_puf_entropy_deinit();
	if (status != AM_HAL_STATUS_SUCCESS) {
		LOG_ERR("Failed to deinitialize PUF entropy, error: 0x%x", status);
		return -EIO;
	}

	return 0;
}

static DEVICE_API(entropy, entropy_ambiq_api_funcs) = {
	.get_entropy = entropy_ambiq_get_trng,
	.get_entropy_isr = entropy_ambiq_get_trng_isr,
};

DEVICE_DT_INST_DEINIT_DEFINE(0, entropy_ambiq_trng_init, entropy_ambiq_trng_deinit, NULL, NULL,
			     NULL, PRE_KERNEL_1, CONFIG_ENTROPY_INIT_PRIORITY,
			     &entropy_ambiq_api_funcs);
