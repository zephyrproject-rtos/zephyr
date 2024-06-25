/*
 * Copyright (c) 2024 Brill Power Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(crc_stm32, CONFIG_CRC_HW_LOG_LEVEL);

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/crc.h>
#include <zephyr/sys/byteorder.h>

#include <soc.h>
#include <stm32_ll_crc.h>

#define DT_DRV_COMPAT st_stm32_crc

struct crc_stm32_cfg {
	CRC_TypeDef *base;
	struct stm32_pclken pclken;
};

struct crc_stm32_data {
	struct k_sem sem;
};

static void crc_lock(const struct device *dev)
{
	struct crc_stm32_data *data = (struct crc_stm32_data *)dev->data;

	k_sem_take(&data->sem, K_FOREVER);
}

static void crc_unlock(const struct device *dev)
{
	struct crc_stm32_data *data = (struct crc_stm32_data *)dev->data;

	k_sem_give(&data->sem);
}

static int crc_stm32_begin(const struct device *dev, struct crc_ctx *ctx)
{
	/* Attempt to take semaphore */
	crc_lock(dev);

	/* Ensure ctx is not currently being updated */
	if (ctx->state == CRC_STATE_IN_PROGRESS) {
		crc_unlock(dev);
		return -EBUSY;
	}

	ctx->state = CRC_STATE_IN_PROGRESS; /* Indicate calculation in progress */

	const struct crc_stm32_cfg *cfg = dev->config;

	LL_CRC_SetInputDataReverseMode(cfg->base, (ctx->flags & CRC_FLAG_REVERSE_INPUT)
							  ? LL_CRC_INDATA_REVERSE_WORD
							  : LL_CRC_INDATA_REVERSE_NONE);
	LL_CRC_SetOutputDataReverseMode(cfg->base, (ctx->flags & CRC_FLAG_REVERSE_OUTPUT)
							   ? LL_CRC_OUTDATA_REVERSE_BIT
							   : LL_CRC_INDATA_REVERSE_NONE);
	LL_CRC_ResetCRCCalculationUnit(cfg->base);

	switch (ctx->type) {
	case CRC8_CCITT_HW: {
		uint8_t init_val = (uint8_t)(ctx->initial_value & 0xFF);
		uint8_t poly = (uint8_t)(ctx->polynomial & 0xFF);

		LL_CRC_SetPolynomialSize(cfg->base, LL_CRC_POLYLENGTH_8B);
		LL_CRC_SetPolynomialCoef(cfg->base, poly);
		LL_CRC_SetInitialData(cfg->base, init_val);
		break;
	}

	case CRC32_IEEE_HW: {
		uint32_t init_val = (uint32_t)(ctx->initial_value & 0xFFFFFFFF);
		uint32_t poly = (uint32_t)(ctx->polynomial & 0xFFFFFFFF);

		LL_CRC_SetPolynomialSize(cfg->base, LL_CRC_POLYLENGTH_32B);
		LL_CRC_SetPolynomialCoef(cfg->base, poly);
		LL_CRC_SetInitialData(cfg->base, init_val);
		break;
	}

	default:
		ctx->state = CRC_STATE_IDLE;
		crc_unlock(dev);
		return -ENOTSUP;
	}
	return 0;
}

static int crc_stm32_update(const struct device *dev, struct crc_ctx *ctx, const void *buffer,
			    size_t bufsize)
{
	/* Ensure CRC calculation has been initialized by crc_begin() */
	if (ctx->state == CRC_STATE_IDLE) {
		return -EINVAL;
	}

	const struct crc_stm32_cfg *cfg = dev->config;
	const uint8_t *buf = buffer;

	switch (ctx->type) {
	case CRC8_CCITT_HW: {
		register uint32_t index = 0;

		for (index = 0; index < bufsize; index++) {
			LL_CRC_FeedData8(cfg->base, buf[index]);
		}
		uint8_t result = LL_CRC_ReadData8(cfg->base);

		ctx->result = (crc_result_t)result;
		break;
	}

	case CRC32_IEEE_HW: {
		register uint32_t crc_data = 0;
		register uint32_t index = 0;

		/* Compute the CRC of Data Buffer array*/
		for (index = 0; index < (bufsize / 4); index++) {
			crc_data = sys_get_le32(buf + 4 * index);
			LL_CRC_FeedData32(cfg->base, crc_data);
		}

		/* Last bytes specific handling */
		if ((bufsize & 3) != 0) {
			if ((bufsize & 3) == 3) {
				LL_CRC_FeedData16(cfg->base, sys_get_le16(buf + 4 * index));
				LL_CRC_FeedData8(cfg->base, buf[4 * index + 2]);
			} else if ((bufsize & 3) == 2) {
				LL_CRC_FeedData16(cfg->base, sys_get_le16(buf + 4 * index));
			} else { /* ((bufsize & 3) == 1) */
				LL_CRC_FeedData8(cfg->base, buf[4 * index]);
			}
		}
		uint32_t result = (~LL_CRC_ReadData32(cfg->base));

		ctx->result = (crc_result_t)result;
		break;
	}

	default:
		ctx->state = CRC_STATE_IDLE;
		crc_unlock(dev);
		return -ENOTSUP;
	}

	return 0;
}

static int crc_stm32_finish(const struct device *dev, struct crc_ctx *ctx)
{
	/* Ensure CRC calculation is in progress */
	if (ctx->state == CRC_STATE_IDLE) {
		return -EINVAL;
	}

	ctx->state = CRC_STATE_IDLE; /* Indicate calculation done */
	crc_unlock(dev);

	return 0;
}

/**
 * @brief Called by Zephyr when instantiating device during boot
 * @param dev Pointer to the device structure for the driver instance.
 * @retval 0       On success
 * @retval -ENODEV Clock control device is not ready
 * @retval -EIO    Fail to turn on appropriate clock for CRC instance
 */
static int crc_stm32_init(const struct device *dev)
{
	const struct crc_stm32_cfg *cfg = dev->config;
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	if (!device_is_ready(clk)) {
		LOG_ERR("CRC: Clock control device not ready");
		return -ENODEV;
	}

	if (clock_control_on(clk, (clock_control_subsys_t *)&cfg->pclken) != 0) {
		LOG_ERR("CRC: Clock control device could not initialise");
		return -EIO;
	}

	struct crc_stm32_data *data = dev->data;

	k_sem_init(&data->sem, 1, 1);

	return 0;
}

static const struct crc_driver_api crc_stm32_driver_api = {
	.crc_begin = crc_stm32_begin,
	.crc_update = crc_stm32_update,
	.crc_finish = crc_stm32_finish,
};

#define STM32_CRC_INIT(index)                                                                      \
                                                                                                   \
	static const struct crc_stm32_cfg crc_stm32_cfg_##index = {                                \
		.base = (CRC_TypeDef *)DT_INST_REG_ADDR(index),                                    \
		.pclken =                                                                          \
			{                                                                          \
				.enr = DT_INST_CLOCKS_CELL(index, bits),                           \
				.bus = DT_INST_CLOCKS_CELL(index, bus),                            \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	static struct crc_stm32_data crc_stm32_data_##index = {};                                  \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, &crc_stm32_init, NULL, &crc_stm32_data_##index,               \
			      &crc_stm32_cfg_##index, POST_KERNEL, CONFIG_CRC_INIT_PRIORITY,       \
			      &crc_stm32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(STM32_CRC_INIT)
