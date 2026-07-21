/*
 * Copyright 2025 Phuc Hoang
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/drivers/crc.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_silabs.h>

#include <sl_hal_gpcrc.h>


LOG_MODULE_REGISTER(silabs_gpcrc, CONFIG_CRC_LOG_LEVEL);

#define DT_DRV_COMPAT silabs_gpcrc

struct crc_silabs_config {
	const struct device *clock_dev;
	const struct silabs_clock_control_cmu_config clock_cfg;
	GPCRC_TypeDef *gpcrc;
};

struct crc_silabs_data {
	bool is_crc32;
	uint32_t xor_out;
	struct k_sem lock;
};

/* Helper func */

static inline void crc_silabs_lock(const struct device *dev)
{
	struct crc_silabs_data *data = dev->data;

	k_sem_take(&data->lock, K_FOREVER);
}

static inline void crc_silabs_unlock(const struct device *dev)
{
	struct crc_silabs_data *data = dev->data;

	k_sem_give(&data->lock);
}

static inline uint16_t reverse_16bit(uint16_t x)
{
	x = ((x & 0xAAAA) >> 1) | ((x & 0x5555) << 1);
	x = ((x & 0xCCCC) >> 2) | ((x & 0x3333) << 2);
	x = ((x & 0xF0F0) >> 4) | ((x & 0x0F0F) << 4);

	return (x >> 8) | (x << 8);
}

static int set_crc_config(const struct device *dev, struct crc_ctx *ctx)
{
	const struct crc_silabs_config *config = dev->config;
	struct crc_silabs_data *data = dev->data;

	/* Check the type, polynomial and init value */
	switch (ctx->type) {
	case CRC16: {
		if (ctx->polynomial != CRC16_POLY) {
			return -EINVAL;
		}
		ctx->seed &= 0xFFFFU;
		data->xor_out = 0x0000U;
		data->is_crc32 = false;
		break;
	}
	case CRC16_ANSI: {
		if (ctx->polynomial != CRC16_REFLECT_POLY) {
			return -EINVAL;
		}
		ctx->seed &= 0xFFFFU;
		data->xor_out = 0x0000U;
		data->is_crc32 = false;
		break;
	}
	case CRC16_ITU_T: {
		__fallthrough;
	}
	case CRC16_CCITT: {
		if (ctx->polynomial != CRC16_CCITT_POLY) {
			return -EINVAL;
		}
		ctx->seed &= 0xFFFFU;
		data->xor_out = 0x0000U;
		data->is_crc32 = false;
		break;
	}
	case CRC32_IEEE: {
		if (ctx->polynomial != CRC32_IEEE_POLY) {
			return -EINVAL;
		}
		data->xor_out = 0xFFFFFFFFU;
		data->is_crc32 = true;
		break;
	}
	default:
		return -ENOTSUP;
	}

	/* Set config for gpcrc */
	const sl_hal_gpcrc_init_t init = {
		/* Depend on the data byte order, if the data byte order is big endian
		 * and we write word/half word to input this flag must be true,
		 * otherwise, it is not.
		 */
		.reverse_byte_order = false,
		/* Flip the flag to get right result
		 * Silabs EFR32 GPCRC needs to use the inverted RefIn and RefOut flags
		 * to produce correct checksum results
		 */
		.reverse_bits = !(ctx->reversed & CRC_FLAG_REVERSE_INPUT), /* reverse input */
		.enable_byte_mode = false,
		.auto_init = false,
		.crc_poly = ctx->polynomial,
		/* 16 bit CRC Seed need to be reversed to get the right result
		 * In the case where the CRC-16 algorithms have asymmetric seed value as follows:
		 * CRC-16/DDS-110
		 * CRC-16/ISO-IEC-14443-3-A
		 * CRC-16/RIELLO
		 * etc
		 */
		.init_value = (data->is_crc32) ? ctx->seed : reverse_16bit(ctx->seed),
	};

	sl_hal_gpcrc_reset(config->gpcrc);
	sl_hal_gpcrc_init(config->gpcrc, &init);

	return 0;
}

static inline void write_data_input(const struct device *dev, const void *buffer, size_t bufsize)
{
	size_t idx = 0;
	const uint8_t *data_pt = (const uint8_t *)buffer;
	const struct crc_silabs_config *config = dev->config;
	struct crc_silabs_data *data = dev->data;

	if (data->is_crc32) {
		/* Word boundaries */
		while ((idx < bufsize) && ((uintptr_t)&data_pt[idx] & 0x3)) {
			sl_hal_gpcrc_write_input_8bit(config->gpcrc, data_pt[idx++]);
		}

		while ((bufsize - idx) >= sizeof(uint32_t)) {
			uint32_t val = *((const uint32_t *)(data_pt + idx));

			sl_hal_gpcrc_write_input_32bit(config->gpcrc, val);
			idx += sizeof(uint32_t);
		}
	} else {
		/* Half word boundaries */
		while ((idx < bufsize) && ((uintptr_t)&data_pt[idx] & 0x1)) {
			sl_hal_gpcrc_write_input_8bit(config->gpcrc, data_pt[idx++]);
		}

		while ((bufsize - idx) >= sizeof(uint16_t)) {
			uint16_t val = *((const uint16_t *)(data_pt + idx));

			sl_hal_gpcrc_write_input_16bit(config->gpcrc, val);
			idx += sizeof(uint16_t);
		}
	}
	/* Handle tail bytes */
	while (idx < bufsize) {
		sl_hal_gpcrc_write_input_8bit(config->gpcrc, data_pt[idx++]);
	}
}

/* Driver APIs */

static int crc_silabs_begin(const struct device *dev, struct crc_ctx *ctx)
{
	int ret;
	const struct crc_silabs_config *config = dev->config;

	if ((ctx == NULL) || (ctx->state != CRC_STATE_IDLE)) {
		return -EINVAL;
	}

	crc_silabs_lock(dev);

	ret = set_crc_config(dev, ctx);
	if (ret != 0) {
		crc_silabs_unlock(dev);
		return ret;
	}

	sl_hal_gpcrc_enable(config->gpcrc);
	sl_hal_gpcrc_start(config->gpcrc);

	ctx->state = CRC_STATE_IN_PROGRESS;

	return 0;
}

static int crc_silabs_update(const struct device *dev, struct crc_ctx *ctx, const void *buffer,
							 size_t bufsize)
{
	uint32_t result = 0;
	const struct crc_silabs_config *config = dev->config;
	struct crc_silabs_data *data = dev->data;
	/* Flip the flag to get right result
	 * Silabs EFR32 GPCRC needs to use the inverted RefIn and RefOut flags
	 * to produce correct checksum results
	 */
	bool reversed_out = !(ctx->reversed & CRC_FLAG_REVERSE_OUTPUT);

	if (ctx->state == CRC_STATE_IDLE) {
		return -EINVAL;
	}
	/* Write data to crc input */
	write_data_input(dev, buffer, bufsize);

	/* Get result */
	if (!reversed_out) {
		result = sl_hal_gpcrc_read_data(config->gpcrc);
	} else {
		result = sl_hal_gpcrc_read_data_bit_reversed(config->gpcrc);
	}
	/* Xor output */
	result ^= data->xor_out;

	if (ctx->type != CRC32_IEEE) {
		ctx->result = (uint16_t)result;
	} else {
		ctx->result = result;
	}

	return 0;
}

static int crc_silabs_finish(const struct device *dev, struct crc_ctx *ctx)
{
	const struct crc_silabs_config *config = dev->config;

	if (ctx->state == CRC_STATE_IDLE) {
		return -EINVAL;
	}

	ctx->state = CRC_STATE_IDLE;

	crc_silabs_unlock(dev);
	sl_hal_gpcrc_disable(config->gpcrc);

	return 0;
}

static DEVICE_API(crc, crc_silabs_driver_api) = {
	.begin = crc_silabs_begin,
	.update = crc_silabs_update,
	.finish = crc_silabs_finish,
};

static int crc_silabs_init(const struct device *dev)
{
	int ret;
	const struct crc_silabs_config *config = dev->config;
	struct crc_silabs_data *data = dev->data;

	if (!device_is_ready(config->clock_dev)) {
		return -ENODEV;
	}

	ret = clock_control_on(config->clock_dev,
		(clock_control_subsys_t)(uintptr_t)&config->clock_cfg);

	if ((ret != 0) && (ret != -EALREADY)) {
		return ret;
	}

	k_sem_init(&data->lock, 1, 1);

	return 0;
}


#define CRC_SILABS_GPCRC_INIT(idx)                                                                \
	static struct crc_silabs_data silabs_gpcrc_data_##idx;                                    \
	static const struct crc_silabs_config silabs_gpcrc_config_##idx = {                       \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(idx)),                             \
		.clock_cfg = SILABS_DT_CLOCK_CFG(DT_DRV_INST(idx)),                               \
		.gpcrc = (GPCRC_TypeDef *)DT_INST_REG_ADDR(idx),                                  \
	};                                                                                        \
	                                                                                          \
	DEVICE_DT_INST_DEFINE(idx,                                                                \
		crc_silabs_init, NULL,                                                            \
		&silabs_gpcrc_data_##idx,                                                         \
		&silabs_gpcrc_config_##idx,                                                       \
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,                                  \
		&crc_silabs_driver_api)                                                           \

DT_INST_FOREACH_STATUS_OKAY(CRC_SILABS_GPCRC_INIT)
