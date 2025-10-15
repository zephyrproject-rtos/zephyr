/*
 * Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/drivers/otp.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/toolchain.h>

#define DT_DRV_COMPAT st_stm32_bsec

LOG_MODULE_REGISTER(otp_bsec_stm32);

#define BSEC_WORD_SIZE	4

static K_MUTEX_DEFINE(lock);

struct bsec_stm32_config {
	BSEC_TypeDef *base;
	unsigned int upper_fuse_limit;
};

static int otp_bsec_stm32_check_accessible(BSEC_HandleTypeDef *handle,
					   const struct bsec_stm32_config *config, off_t offset,
					   unsigned int nb_fuse)
{
	uint32_t fuse_idx = offset / BSEC_WORD_SIZE;
	HAL_StatusTypeDef hal_ret;
	uint32_t bsec_state = 0;

	if (nb_fuse == 0) {
		return -EINVAL;
	}

	hal_ret = HAL_BSEC_GetDeviceLifeCycleState(handle, &bsec_state);
	if (hal_ret != HAL_OK) {
		return -EACCES;
	}

	/* Upper fuses are only accessible when the BSEC is in closed locked state */
	if (((fuse_idx + nb_fuse) > config->upper_fuse_limit) &&
	    (bsec_state != HAL_BSEC_CLOSED_STATE)) {
		return -EACCES;
	}

	return 0;
}

#if defined(CONFIG_OTP_PROGRAM)
static int otp_bsec_stm32_program(const struct device *dev, off_t offset, const void *buf,
				  size_t len)
{
	const struct bsec_stm32_config *config = dev->config;
	BSEC_HandleTypeDef handle = { .Instance = config->base };
	HAL_StatusTypeDef hal_ret;
	unsigned int nb_fuse;
	unsigned int i;
	int ret;

	/* Allow programming of 4bytes words only */
	if (!IS_ALIGNED(len, BSEC_WORD_SIZE)) {
		LOG_ERR("Invalid length to program OTP: %zu", len);
		return -EINVAL;
	}

	/* Allow programming only at the beginning of a new word */
	if (!IS_ALIGNED(offset, BSEC_WORD_SIZE)) {
		LOG_ERR("Programmed data not aligned on an OTP word");
		return -EINVAL;
	}

	nb_fuse = len / BSEC_WORD_SIZE;

	ret = otp_bsec_stm32_check_accessible(&handle, config, offset, nb_fuse);
	if (ret != 0) {
		return ret;
	}

	k_mutex_lock(&lock, K_FOREVER);

	for (i = 0; i < nb_fuse; i++) {
		uint32_t prog_data = 0;

		LOG_DBG("Programming Fuse %lu", (offset / BSEC_WORD_SIZE) + i);

		prog_data = UNALIGNED_GET((uint32_t *)((uint8_t *)buf + i * BSEC_WORD_SIZE));

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
	uint8_t *dest = (uint8_t *)buf;
	HAL_StatusTypeDef hal_ret;
	size_t bytes_left = len;
	unsigned int nb_fuse;
	unsigned int i;
	int ret;

	/* Allow intra-word and spanned reads but not 0-sized reads */
	nb_fuse = len != 0 ? DIV_ROUND_UP(offset % BSEC_WORD_SIZE + len, BSEC_WORD_SIZE) : 0;

	ret = otp_bsec_stm32_check_accessible(&handle, config, offset, nb_fuse);
	if (ret != 0) {
		return ret;
	}

	k_mutex_lock(&lock, K_FOREVER);

	for (i = 0; i < nb_fuse; i++) {
		size_t first_offset  = (i == 0) ? offset % BSEC_WORD_SIZE : 0;
		size_t read_sz = MIN(BSEC_WORD_SIZE - first_offset, bytes_left);
		uint32_t fuse_data = 0;

		LOG_DBG("Reading Fuse %lu", (offset / BSEC_WORD_SIZE) + i);

		hal_ret = HAL_BSEC_OTP_Read(&handle, (offset / BSEC_WORD_SIZE) + i, &fuse_data);
		if (hal_ret != HAL_OK) {
			k_mutex_unlock(&lock);
			return -EACCES;
		}

		memcpy(dest, ((uint8_t *)&fuse_data) + first_offset, read_sz);
		dest += read_sz;
		bytes_left -= read_sz;
		if (bytes_left == 0) {
			break;
		}
	}

	k_mutex_unlock(&lock);

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

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, &bsec_config, PRE_KERNEL_1,
		      CONFIG_OTP_INIT_PRIORITY, &otp_bsec_stm32_api);
