/*
 * Copyright 2025 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpc_crc

#include <zephyr/drivers/crc.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(nxp_lpc_crc, CONFIG_CRC_LOG_LEVEL);

#include <fsl_crc.h>

struct crc_nxp_lpc_config {
	CRC_Type *base;
};

struct crc_nxp_lpc_data {
#ifdef CONFIG_MULTITHREADING
	struct k_sem lock;
#endif
};

static inline void crc_nxp_lpc_lock(const struct device *dev)
{
#ifdef CONFIG_MULTITHREADING
	struct crc_nxp_lpc_data *data = dev->data;

	k_sem_take(&data->lock, K_FOREVER);

#else
	ARG_UNUSED(dev);

#endif
}

static inline void crc_nxp_lpc_unlock(const struct device *dev)
{
#ifdef CONFIG_MULTITHREADING
	struct crc_nxp_lpc_data *data = dev->data;

	k_sem_give(&data->lock);

#else
	ARG_UNUSED(dev);

#endif
}

static int crc_nxp_lpc_prepare_config(const struct crc_ctx *ctx, crc_config_t *cfg)
{
	if ((ctx == NULL) || (cfg == NULL)) {
		return -EINVAL;
	}

	cfg->reverseIn = (ctx->reversed & CRC_FLAG_REVERSE_INPUT) != 0U;
	cfg->reverseOut = (ctx->reversed & CRC_FLAG_REVERSE_OUTPUT) != 0U;
	cfg->complementIn = false;
	cfg->complementOut = false;
	cfg->seed = ctx->seed;

	switch (ctx->type) {
	case CRC16:
		if (ctx->polynomial != CRC16_POLY) {
			return -EINVAL;
		}
		cfg->polynomial = kCRC_Polynomial_CRC_16;
		cfg->seed &= 0xFFFFU;
		break;
	case CRC16_CCITT:
		if (ctx->polynomial != CRC16_CCITT_POLY) {
			return -EINVAL;
		}
		cfg->polynomial = kCRC_Polynomial_CRC_CCITT;
		cfg->seed &= 0xFFFFU;
		break;
	case CRC32_IEEE:
		if (ctx->polynomial != CRC32_IEEE_POLY) {
			return -EINVAL;
		}
		cfg->polynomial = kCRC_Polynomial_CRC_32;
		/* IEEE uses final XOR */
		cfg->complementOut = true;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int crc_nxp_lpc_begin(const struct device *dev, struct crc_ctx *ctx)
{
	const struct crc_nxp_lpc_config *config = dev->config;
	crc_config_t cfg;
	int ret;

	if ((ctx == NULL) || (ctx->state != CRC_STATE_IDLE)) {
		return -EINVAL;
	}

	crc_nxp_lpc_lock(dev);

	ret = crc_nxp_lpc_prepare_config(ctx, &cfg);
	if (ret != 0) {
		crc_nxp_lpc_unlock(dev);
		return ret;
	}

	CRC_Init(config->base, &cfg);

	ctx->state = CRC_STATE_IN_PROGRESS;
	ctx->result = 0U;

	return 0;
}

static int crc_nxp_lpc_update(const struct device *dev, struct crc_ctx *ctx, const void *buffer,
			      size_t bufsize)
{
	const struct crc_nxp_lpc_config *config = dev->config;

	if (ctx == NULL) {
		return -EINVAL;
	}

	if (ctx->state != CRC_STATE_IN_PROGRESS) {
		return -EINVAL;
	}

	/* Validate buffer pointer when bufsize is non-zero */
	if ((bufsize > 0U) && (buffer == NULL)) {
		ctx->state = CRC_STATE_IDLE;
		crc_nxp_lpc_unlock(dev);
		return -EINVAL;
	}

	/* Process data only if bufsize > 0 (allows zero-length updates) */
	if (bufsize > 0U) {
		/*
		 * Hardware processes data in 8-bit chunks internally.
		 * 32-bit writes take 4 cycles (8-bit x 4).
		 */
		CRC_WriteData(config->base, (const uint8_t *)buffer, bufsize);
	}

	/* Keep an updated result for streaming verification */
	if (ctx->type == CRC32_IEEE) {
		ctx->result = CRC_Get32bitResult(config->base);
	} else {
		ctx->result = (uint32_t)CRC_Get16bitResult(config->base);
	}

	return 0;
}

static int crc_nxp_lpc_finish(const struct device *dev, struct crc_ctx *ctx)
{
	const struct crc_nxp_lpc_config *config = dev->config;

	if (ctx == NULL) {
		return -EINVAL;
	}

	if (ctx->state != CRC_STATE_IN_PROGRESS) {
		return -EINVAL;
	}

	if (ctx->type == CRC32_IEEE) {
		ctx->result = CRC_Get32bitResult(config->base);
	} else {
		ctx->result = (uint32_t)CRC_Get16bitResult(config->base);
	}

	ctx->state = CRC_STATE_IDLE;
	crc_nxp_lpc_unlock(dev);
	return 0;
}

static DEVICE_API(crc, crc_nxp_lpc_driver_api) = {
	.begin = crc_nxp_lpc_begin,
	.update = crc_nxp_lpc_update,
	.finish = crc_nxp_lpc_finish,
};

static int crc_nxp_lpc_init(const struct device *dev)
{
#ifdef CONFIG_MULTITHREADING
	struct crc_nxp_lpc_data *data = dev->data;
	int ret;

	ret = k_sem_init(&data->lock, 1, 1);
	if (ret != 0) {
		return ret;
	}

#else
	ARG_UNUSED(dev);

#endif
	return 0;
}

#define CRC_NXP_LPC_INIT(inst)                                                                     \
	static struct crc_nxp_lpc_data crc_nxp_lpc_data_##inst;                                    \
	static const struct crc_nxp_lpc_config crc_nxp_lpc_config_##inst = {                       \
		.base = (CRC_Type *)DT_INST_REG_ADDR(inst),                                        \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, crc_nxp_lpc_init, NULL, &crc_nxp_lpc_data_##inst,              \
			      &crc_nxp_lpc_config_##inst, POST_KERNEL,                             \
			      CONFIG_CRC_DRIVER_INIT_PRIORITY, &crc_nxp_lpc_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CRC_NXP_LPC_INIT)
