/*
 * Copyright (c) 2025 SiFli Technologies(Nanjing) Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb_crc

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control/sf32lb.h>
#include <zephyr/drivers/crc.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

#include <register.h>

#define CRC_DR_OFFSET   offsetof(CRC_TypeDef, DR)
#define CRC_SR_OFFSET   offsetof(CRC_TypeDef, SR)
#define CRC_CR_OFFSET   offsetof(CRC_TypeDef, CR)
#define CRC_INIT_OFFSET offsetof(CRC_TypeDef, INIT)
#define CRC_POL_OFFSET  offsetof(CRC_TypeDef, POL)

#define CRC_POLYSIZE_32 0U
#define CRC_POLYSIZE_16 1U
#define CRC_POLYSIZE_8  2U
#define CRC_POLYSIZE_7  3U

#define CRC_DATASIZE_8 0U

#define CRC_REV_IN_BYTE 1U

/* Poll timeout in microseconds (10 ms) */
#define CRC_SF32LB_TIMEOUT_US 10000U

struct crc_sf32lb_config {
	uintptr_t base;
	struct sf32lb_clock_dt_spec clock;
};

struct crc_sf32lb_data {
	struct k_sem lock;
	uint8_t width;
	uint32_t xor_out;
};

static inline uint32_t crc_sf32lb_mask(uint8_t width)
{
	if (width >= 32U) {
		return UINT32_MAX;
	}

	return BIT_MASK(width);
}

static int crc_sf32lb_prepare_config(const struct crc_ctx *ctx, uint8_t *polysize, uint8_t *width,
				     uint32_t *xor_out)
{
	switch (ctx->type) {
	case CRC8:
		__fallthrough;
	case CRC8_CCITT:
		__fallthrough;
	case CRC8_ROHC:
		if (ctx->polynomial > UINT8_MAX) {
			return -EINVAL;
		}

		*polysize = CRC_POLYSIZE_8;
		*width = 8U;
		*xor_out = 0U;
		break;
	case CRC16:
		__fallthrough;
	case CRC16_CCITT:
		__fallthrough;
	case CRC16_ANSI:
		__fallthrough;
	case CRC16_ITU_T:
		if (ctx->polynomial > UINT16_MAX) {
			return -EINVAL;
		}

		*polysize = CRC_POLYSIZE_16;
		*width = 16U;
		*xor_out = 0U;
		break;
	case CRC32_IEEE:
		*polysize = CRC_POLYSIZE_32;
		*width = 32U;
		*xor_out = 0xFFFFFFFFU;
		break;
	case CRC32_C:
		__fallthrough;
	case CRC32_K_4_2:
		*polysize = CRC_POLYSIZE_32;
		*width = 32U;
		*xor_out = 0U;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static void crc_sf32lb_unlock(const struct device *dev)
{
	struct crc_sf32lb_data *data = dev->data;

	k_sem_give(&data->lock);
}

static uint32_t crc_sf32lb_get_result(const struct device *dev)
{
	const struct crc_sf32lb_config *config = dev->config;
	struct crc_sf32lb_data *data = dev->data;
	uint32_t raw;
	uint32_t mask;

	raw = sys_read32(config->base + CRC_DR_OFFSET) ^ data->xor_out;
	mask = crc_sf32lb_mask(data->width);

	return raw & mask;
}

static int crc_sf32lb_begin(const struct device *dev, struct crc_ctx *ctx)
{
	const struct crc_sf32lb_config *config = dev->config;
	struct crc_sf32lb_data *data = dev->data;
	uint8_t polysize;
	uint8_t width;
	uint32_t xor_out;
	uint32_t cr;
	uint32_t mask;
	int ret;

	if ((ctx == NULL) || (ctx->state != CRC_STATE_IDLE)) {
		return -EINVAL;
	}

	k_sem_take(&data->lock, K_FOREVER);

	ret = crc_sf32lb_prepare_config(ctx, &polysize, &width, &xor_out);
	if (ret != 0) {
		crc_sf32lb_unlock(dev);
		return ret;
	}

	data->width = width;
	data->xor_out = xor_out;
	mask = crc_sf32lb_mask(width);

	cr = FIELD_PREP(CRC_CR_POLYSIZE_Msk, polysize);

	if ((ctx->reversed & CRC_FLAG_REVERSE_INPUT) != 0U) {
		cr |= FIELD_PREP(CRC_CR_REV_IN_Msk, CRC_REV_IN_BYTE);
	}

	if ((ctx->reversed & CRC_FLAG_REVERSE_OUTPUT) != 0U) {
		cr |= CRC_CR_REV_OUT;
	}

	sys_write32(cr, config->base + CRC_CR_OFFSET);
	sys_write32(ctx->seed & mask, config->base + CRC_INIT_OFFSET);
	sys_write32(ctx->polynomial & mask, config->base + CRC_POL_OFFSET);

	/* Reset data register to the provided seed */
	sys_write32(cr | CRC_CR_RESET, config->base + CRC_CR_OFFSET);
	sys_write32(cr, config->base + CRC_CR_OFFSET);

	ctx->state = CRC_STATE_IN_PROGRESS;
	ctx->result = ctx->seed & mask;

	return 0;
}

static int crc_sf32lb_update(const struct device *dev, struct crc_ctx *ctx, const void *buffer,
			     size_t bufsize)
{
	const struct crc_sf32lb_config *config = dev->config;
	const uint8_t *data_buf = buffer;

	if (ctx->state != CRC_STATE_IN_PROGRESS) {
		return -EINVAL;
	}

	if ((bufsize > 0U) && (data_buf == NULL)) {
		ctx->state = CRC_STATE_IDLE;
		crc_sf32lb_unlock(dev);
		return -EINVAL;
	}

	size_t aligned_len = ROUND_DOWN(bufsize, sizeof(uint32_t));
	size_t idx = 0U;

	for (; idx < aligned_len; idx += sizeof(uint32_t)) {
		sys_set_bits(config->base + CRC_CR_OFFSET, CRC_CR_DATASIZE_Msk);
		uint32_t data = sys_get_le32(&data_buf[idx]);

		sys_write32(data, config->base + CRC_DR_OFFSET);

		if (!WAIT_FOR((sys_read32(config->base + CRC_SR_OFFSET) & CRC_SR_DONE) != 0U,
			      CRC_SF32LB_TIMEOUT_US, NULL)) {
			ctx->state = CRC_STATE_IDLE;
			crc_sf32lb_unlock(dev);
			return -ETIMEDOUT;
		}
	}

	/* Now we'll handle data that isn't 4-byte aligned */
	if (idx < bufsize) {
		uint32_t rem = 0U;
		size_t rem_bytes = bufsize - idx;

		sys_clear_bits(config->base + CRC_CR_OFFSET, CRC_CR_DATASIZE_Msk);
		sys_set_bits(config->base + CRC_CR_OFFSET,
			     FIELD_PREP(CRC_CR_DATASIZE_Msk, (rem_bytes - 1U)));

		sys_get_le(&rem, &data_buf[idx], rem_bytes);

		sys_write32(rem, config->base + CRC_DR_OFFSET);

		if (!WAIT_FOR((sys_read32(config->base + CRC_SR_OFFSET) & CRC_SR_DONE) != 0U,
			      CRC_SF32LB_TIMEOUT_US, NULL)) {
			ctx->state = CRC_STATE_IDLE;
			crc_sf32lb_unlock(dev);
			return -ETIMEDOUT;
		}
	}

	ctx->result = crc_sf32lb_get_result(dev);

	return 0;
}

static int crc_sf32lb_finish(const struct device *dev, struct crc_ctx *ctx)
{
	if (ctx->state != CRC_STATE_IN_PROGRESS) {
		return -EINVAL;
	}

	ctx->result = crc_sf32lb_get_result(dev);
	ctx->state = CRC_STATE_IDLE;

	crc_sf32lb_unlock(dev);

	return 0;
}

static DEVICE_API(crc, crc_sf32lb_driver_api) = {
	.begin = crc_sf32lb_begin,
	.update = crc_sf32lb_update,
	.finish = crc_sf32lb_finish,
};

static int crc_sf32lb_init(const struct device *dev)
{
	const struct crc_sf32lb_config *config = dev->config;
	struct crc_sf32lb_data *data = dev->data;
	int ret;

	if (!sf32lb_clock_is_ready_dt(&config->clock)) {
		return -ENODEV;
	}

	ret = sf32lb_clock_control_on_dt(&config->clock);
	if (ret != 0) {
		return ret;
	}

	k_sem_init(&data->lock, 1, 1);
	data->width = 32U;
	data->xor_out = 0U;

	return 0;
}

#define CRC_SF32LB_INIT(inst)                                                                      \
	static struct crc_sf32lb_data crc_sf32lb_data_##inst;                                      \
	static const struct crc_sf32lb_config crc_sf32lb_config_##inst = {                         \
		.base = DT_INST_REG_ADDR(inst),                                                    \
		.clock = SF32LB_CLOCK_DT_INST_SPEC_GET(inst),                                      \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, crc_sf32lb_init, NULL, &crc_sf32lb_data_##inst,                \
			      &crc_sf32lb_config_##inst, POST_KERNEL,                              \
			      CONFIG_CRC_DRIVER_INIT_PRIORITY, &crc_sf32lb_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CRC_SF32LB_INIT)
