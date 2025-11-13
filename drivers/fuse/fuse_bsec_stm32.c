/*
 * Copyright (c) 2025 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/drivers/fuse.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define DT_DRV_COMPAT st_stm32_bsec

LOG_MODULE_REGISTER(fuse_bsec_stm32);

#define BSEC_WORD_SIZE	4

static K_MUTEX_DEFINE(lock);

struct bsec_stm32_config {
	unsigned int upper_fuse_limit;
};

static int fuse_bsec_stm32_program(const struct device *dev, off_t offset, const void *buf,
				   size_t len)
{
	const struct bsec_stm32_config *config = dev->config;
	unsigned int base_otp = offset / BSEC_WORD_SIZE;
	HAL_StatusTypeDef hal_ret = HAL_OK;
	BSEC_HandleTypeDef handle = {0};
	unsigned int nb_fuse;
	uint32_t bsec_state;
	unsigned int i;

	if (!IS_ENABLED(CONFIG_FUSE_PROGRAM)) {
		return -ENOTSUP;
	}

	/* Allow programming of 4bytes words only */
	if (len == 0U || len % BSEC_WORD_SIZE) {
		LOG_ERR("Invalid length to program OTP: %zu", len);
		return -EINVAL;
	}

	/* Allow programming only at the beginning of a new word */
	if (!IS_ALIGNED(offset, BSEC_WORD_SIZE)) {
		LOG_ERR("Programmed data not aligned on an OTP word");
		return -EINVAL;
	}

	handle.Instance = BSEC;

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

		memcpy(&prog_data, (uint32_t *)buf + i, BSEC_WORD_SIZE);

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

static int fuse_bsec_stm32_read(const struct device *dev, off_t offset, void *buf, size_t len)
{
	const struct bsec_stm32_config *config = dev->config;
	unsigned int base_otp = offset / BSEC_WORD_SIZE;
	HAL_StatusTypeDef hal_ret = HAL_OK;
	BSEC_HandleTypeDef handle = {0};
	unsigned int nb_fuse;
	uint32_t bsec_state;
	unsigned int i;

	if (len == 0U) {
		LOG_ERR("Read OTP with a len of 0");
		return -EPERM;
	}

	/* Allow reading only at the beginning of a new word */
	if (!IS_ALIGNED(offset, BSEC_WORD_SIZE)) {
		LOG_ERR("Read data not aligned on an OTP word");
		return -EINVAL;
	}

	handle.Instance = BSEC;

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
		size_t read_sz = (len - i * BSEC_WORD_SIZE) >= BSEC_WORD_SIZE ?
				 BSEC_WORD_SIZE : (len - i * BSEC_WORD_SIZE);

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
	return 0;
}

static const struct bsec_stm32_config eeprom_config = {
	.upper_fuse_limit = DT_INST_PROP(0, st_upper_fuse_limit),
};

static DEVICE_API(fuse, fuse_bsec_stm32_api) = {
	.program = fuse_bsec_stm32_program,
	.read = fuse_bsec_stm32_read,
};

DEVICE_DT_INST_DEFINE(0, bsec_initialize, NULL, NULL, &eeprom_config, PRE_KERNEL_1,
		      CONFIG_FUSE_INIT_PRIORITY, &fuse_bsec_stm32_api);
