/*
 * Copyright (c) 2025 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/drivers/otp.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define DT_DRV_COMPAT st_stm32_bsec

LOG_MODULE_REGISTER(otp_bsec_stm32);

#define BSEC_WORD_SIZE	4

static K_MUTEX_DEFINE(lock);

struct bsec_stm32_config {
	BSEC_TypeDef *base;
	unsigned int upper_fuse_limit;
};

#if defined(CONFIG_OTP_PROGRAM)
static int otp_bsec_stm32_program(const struct device *dev, off_t offset, const void *buf,
				  size_t len)
{
	const struct bsec_stm32_config *config = dev->config;
	BSEC_HandleTypeDef handle = { .Instance = config->base };
	unsigned int base_otp = offset / BSEC_WORD_SIZE;
	HAL_StatusTypeDef hal_ret = HAL_OK;
	unsigned int nb_fuse;
	uint32_t bsec_state;
	unsigned int i;

	/* Allow programming of 4bytes words only */
	if (!IS_ALIGNED(len, BSEC_WORD_SIZE)) {
		LOG_ERR("Invalid length to program OTP: %zu", len);
		return -EINVAL;
	}

	/* Allow programming only at the beginning of a new word */
	if (!IS_ALIGNED(offset, BSEC_WORD_SIZE) || !IS_ALIGNED(buf, BSEC_WORD_SIZE)) {
		LOG_ERR("Programmed data not aligned on an OTP word");
		return -EINVAL;
	}

	hal_ret = HAL_BSEC_GetDeviceLifeCycleState(&handle, &bsec_state);
	if (hal_ret != HAL_OK) {
		return -EACCES;
	}

	/* Upper fuses are only accessible when the BSEC is in closed locked state */
	if (base_otp >= config->upper_fuse_limit && bsec_state != HAL_BSEC_CLOSED_STATE) {
		return -EACCES;
	}

	nb_fuse = len / BSEC_WORD_SIZE;

	k_mutex_lock(&lock, K_FOREVER);

	for (i = 0; i < nb_fuse; i++) {
		uint32_t prog_data = 0;

		LOG_DBG("Programming Fuse %lu", (offset / BSEC_WORD_SIZE) + i);

		memcpy(&prog_data, (const uint32_t *)buf + i, BSEC_WORD_SIZE);

		hal_ret = HAL_BSEC_OTP_Program(&handle, (offset / BSEC_WORD_SIZE) + i, prog_data,
					       0);
		if (hal_ret != HAL_OK) {
			k_mutex_unlock(&lock);
			return -EACCES;
		}
	}

	k_mutex_unlock(&lock);

	return 0;
}
#endif /* CONFIG_OTP_PROGRAM */

static int otp_bsec_stm32_read(const struct device *dev, off_t offset, void *buf, size_t len)
{
	const struct bsec_stm32_config *config = dev->config;
	BSEC_HandleTypeDef handle = { .Instance = config->base };
	unsigned int base_otp = offset / BSEC_WORD_SIZE;
	HAL_StatusTypeDef hal_ret = HAL_OK;
	unsigned int nb_fuse;
	uint32_t bsec_state;
	unsigned int i;

	/* Allow reading only at the beginning of a new word */
	if (!IS_ALIGNED(offset, BSEC_WORD_SIZE)) {
		LOG_ERR("Read data not aligned on an OTP word");
		return -EINVAL;
	}

	hal_ret = HAL_BSEC_GetDeviceLifeCycleState(&handle, &bsec_state);
	if (hal_ret != HAL_OK) {
		return -EACCES;
	}

	/* Upper fuses are only accessible when the BSEC is in closed locked state */
	if (base_otp >= config->upper_fuse_limit && bsec_state != HAL_BSEC_CLOSED_STATE) {
		return -EACCES;
	}

	nb_fuse = DIV_ROUND_UP(len, BSEC_WORD_SIZE);

	k_mutex_lock(&lock, K_FOREVER);

	for (i = 0; i < nb_fuse; i++) {
		uint32_t fuse_data = 0;
		size_t read_sz = MIN(len - i * BSEC_WORD_SIZE, BSEC_WORD_SIZE);

		LOG_DBG("Reading Fuse %lu", (offset / BSEC_WORD_SIZE) + i);

		hal_ret = HAL_BSEC_OTP_Read(&handle, (offset / BSEC_WORD_SIZE) + i, &fuse_data);
		if (hal_ret != HAL_OK) {
			k_mutex_unlock(&lock);
			return -EACCES;
		}

		memcpy((uint32_t *)buf + i, &fuse_data, read_sz);
	}

	k_mutex_unlock(&lock);

	return 0;
}


static int bsec_initialize(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static const struct bsec_stm32_config bsec_config = {
	.base = (void *)DT_INST_REG_ADDR(0),
	.upper_fuse_limit = DT_INST_PROP(0, st_upper_fuse_limit),
};

static DEVICE_API(otp, otp_bsec_stm32_api) = {
#if defined(CONFIG_OTP_PROGRAM)
	.program = otp_bsec_stm32_program,
#endif
	.read = otp_bsec_stm32_read,
};

DEVICE_DT_INST_DEFINE(0, bsec_initialize, NULL, NULL, &bsec_config, PRE_KERNEL_1,
		      CONFIG_OTP_INIT_PRIORITY, &otp_bsec_stm32_api);
