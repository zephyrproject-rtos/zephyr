/*
 * Copyright (c) 2025 Ambiq Micro, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ambiq_puf_trng

#include <string.h>
#include "soc.h"
#include <zephyr/logging/log.h>
#include <zephyr/drivers/entropy.h>

LOG_MODULE_REGISTER(ambiq_puf_trng_entropy, CONFIG_ENTROPY_LOG_LEVEL);

/*
 * Select the correct TRNG memory base address.
 *
 * The TRNG is implemented in the OTP memory
 * and the TRNG address is memory mapped to the OTP.
 *
 */
#define TRNG_BASE DT_INST_REG_ADDR(0)

/* Max Fail Count  for errors */
#define MAX_FAIL_COUNT 5

static inline uint32_t get_trng_u32(void)
{
	return AM_REGVAL(TRNG_BASE);
}

static int entropy_ambiq_get_trng(const struct device *dev, uint8_t *buffer, uint16_t length)
{
	ARG_UNUSED(dev);

	/* Validate input parameters */
	if (length == 0 || buffer == NULL) {
		return -EINVAL;
	}

	uint8_t *byte_buffer = buffer;
	uint8_t fail_cnt = 0;

	/*
	 * While the passed in length is greater than zero
	 * grab data from RNG and save to output.
	 */
	while ((length > 0) && (fail_cnt < MAX_FAIL_COUNT)) {
		uint32_t word = get_trng_u32();

		/* This is a failure mode where the RNG doesn't have enough randomness */
		if (word == 0xdeaddead) {
			fail_cnt++;
			continue;
		}

		size_t copy_length = MIN(sizeof(uint32_t), length);
		/*
		 * If the length is less than 4 bytes, we only copy the
		 * requested number of bytes.
		 */
		memcpy(byte_buffer, &word, copy_length);
		byte_buffer += copy_length;
		length -= copy_length;
	}

	if (fail_cnt >= MAX_FAIL_COUNT) {
		return -EIO;
	}

	return 0;
}

static int entropy_ambiq_trng_init(const struct device *dev)
{
	uint32_t status;
	bool peripheral_enabled = false;

	/* Check and Power on OTP if it is not already on. */
	status = am_hal_pwrctrl_periph_enabled(AM_HAL_PWRCTRL_PERIPH_OTP, &peripheral_enabled);
	if (status != AM_HAL_STATUS_SUCCESS) {
		LOG_ERR("Failed to check OTP peripheral status, error: 0x%x", status);
		return -EBUSY;
	}

	if (!peripheral_enabled) {
		status = am_hal_pwrctrl_periph_enable(AM_HAL_PWRCTRL_PERIPH_OTP);
		if (status != AM_HAL_STATUS_SUCCESS) {
			LOG_ERR("Failed to enable OTP peripheral, error: 0x%x", status);
			return -EBUSY;
		}
	}

	return 0;
}

static DEVICE_API(entropy, entropy_ambiq_api_funcs) = {
	.get_entropy = entropy_ambiq_get_trng,
};

DEVICE_DT_INST_DEFINE(0, entropy_ambiq_trng_init, NULL, NULL, NULL, PRE_KERNEL_1,
			  CONFIG_ENTROPY_INIT_PRIORITY, &entropy_ambiq_api_funcs);
