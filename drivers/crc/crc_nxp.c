/*
 * Copyright 2025 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_crc

#include <zephyr/drivers/crc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nxp_crc, CONFIG_CRC_LOG_LEVEL);

/* Namespace collision process: there are crc_result_t defined in both mcux and Zephyr */
#define crc_result_t mcux_crc_result_t
#include "fsl_crc.h"
#undef crc_result_t

struct crc_nxp_config {
	CRC_Type *base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	struct reset_dt_spec reset;
};

struct crc_nxp_data {
	struct k_sem lock;
};

static inline void crc_nxp_lock(const struct device *dev)
{
	struct crc_nxp_data *data = dev->data;

	k_sem_take(&data->lock, K_FOREVER);
}

static inline void crc_nxp_unlock(const struct device *dev)
{
	struct crc_nxp_data *data = dev->data;

	k_sem_give(&data->lock);
}

static int crc_nxp_prepare_config(const struct device *dev, const struct crc_ctx *ctx,
				  crc_config_t *cfg, bool *use32)
{
	const struct crc_nxp_config *config = dev->config;

	ARG_UNUSED(config);

	if ((ctx == NULL) || (cfg == NULL)) {
		return -EINVAL;
	}

	cfg->polynomial = ctx->polynomial;
	cfg->seed = ctx->seed;
	cfg->reflectIn = (ctx->reversed & CRC_FLAG_REVERSE_INPUT) != 0U;
	cfg->reflectOut = (ctx->reversed & CRC_FLAG_REVERSE_OUTPUT) != 0U;
	cfg->complementChecksum = false;
	cfg->crcResult = kCrcFinalChecksum;

	switch (ctx->type) {
	case CRC16:
		/*
		 * The CRC engine takes the polynomial in normal (MSBit-first) form
		 * and performs input/output reflection through reflectIn/reflectOut.
		 * crc16() passes the normal CRC-16 polynomial (0x8005), while
		 * crc16_reflect()/crc16_ansi() pass its reflected form (0xA001) with
		 * both reflect flags set. The reflected form is only accepted when
		 * both input and output are reflected -- the only case where it can
		 * be honored by programming the equivalent normal-form polynomial;
		 * accepting it otherwise would silently compute a different CRC.
		 */
		if (ctx->polynomial == CRC16_REFLECT_POLY) {
			if ((ctx->reversed & (CRC_FLAG_REVERSE_INPUT | CRC_FLAG_REVERSE_OUTPUT)) !=
			    (CRC_FLAG_REVERSE_INPUT | CRC_FLAG_REVERSE_OUTPUT)) {
				return -EINVAL;
			}
			/* Program the equivalent normal-form polynomial. */
			cfg->polynomial = CRC16_POLY;
		} else if (ctx->polynomial != CRC16_POLY) {
			return -EINVAL;
		}
		cfg->seed &= 0xFFFFU;
		cfg->crcBits = kCrcBits16;
		*use32 = false;
		break;
	case CRC16_CCITT:
	case CRC16_ITU_T:
		if (ctx->polynomial != CRC16_CCITT_POLY) {
			return -EINVAL;
		}
		cfg->seed &= 0xFFFFU;
		cfg->crcBits = kCrcBits16;
		*use32 = false;
		break;
	case CRC16_ANSI:
		if (ctx->polynomial != CRC16_REFLECT_POLY) {
			return -EINVAL;
		}
		cfg->seed &= 0xFFFFU;
		cfg->crcBits = kCrcBits16;
		*use32 = false;
		break;
	case CRC32_C:
		if (ctx->polynomial != CRC32C_POLY) {
			return -EINVAL;
		}
		cfg->crcBits = kCrcBits32;
		*use32 = true;
		break;
	case CRC32_IEEE:
		if (ctx->polynomial != CRC32_IEEE_POLY) {
			return -EINVAL;
		}
		cfg->crcBits = kCrcBits32;
		cfg->complementChecksum = true; /* IEEE requires final XOR */
		*use32 = true;
		break;
	case CRC32_MPEG2:
		if (ctx->polynomial != CRC32_IEEE_POLY) {
			return -EINVAL;
		}
		cfg->crcBits = kCrcBits32;
		/* MPEG-2 uses no input/output reflection and no final XOR */
		*use32 = true;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int crc_nxp_begin(const struct device *dev, struct crc_ctx *ctx)
{
	const struct crc_nxp_config *config = dev->config;
	crc_config_t cfg;
	bool use32 = false;
	int ret;

	if ((ctx == NULL) || (ctx->state != CRC_STATE_IDLE)) {
		return -EINVAL;
	}

	crc_nxp_lock(dev);

	ret = crc_nxp_prepare_config(dev, ctx, &cfg, &use32);
	if (ret != 0) {
		crc_nxp_unlock(dev);
		return ret;
	}

	/* Initialize hardware with protocol settings and seed */
	CRC_Init(config->base, &cfg);

	ctx->state = CRC_STATE_IN_PROGRESS;

	return 0;
}

static int crc_nxp_update(const struct device *dev, struct crc_ctx *ctx, const void *buffer,
			  size_t bufsize)
{
	const struct crc_nxp_config *config = dev->config;

	if (ctx->state != CRC_STATE_IN_PROGRESS) {
		return -EINVAL;
	}

	/* Allow zero-length updates */
	if ((bufsize > 0U) && (buffer == NULL)) {
		ctx->state = CRC_STATE_IDLE;
		crc_nxp_unlock(dev);
		return -EINVAL;
	}

	if (bufsize > 0U) {
		CRC_WriteData(config->base, (const uint8_t *)buffer, bufsize);
	}

	/* Keep an updated result for streaming verification */
	if ((ctx->type == CRC32_C) || (ctx->type == CRC32_IEEE) ||
	    (ctx->type == CRC32_MPEG2)) {
		ctx->result = CRC_Get32bitResult(config->base);
	} else {
		ctx->result = (uint32_t)CRC_Get16bitResult(config->base);
	}

	return 0;
}

static int crc_nxp_finish(const struct device *dev, struct crc_ctx *ctx)
{
	const struct crc_nxp_config *config = dev->config;

	if (ctx->state != CRC_STATE_IN_PROGRESS) {
		return -EINVAL;
	}

	/* Read final result */
	if ((ctx->type == CRC32_C) || (ctx->type == CRC32_IEEE) ||
	    (ctx->type == CRC32_MPEG2)) {
		ctx->result = CRC_Get32bitResult(config->base);
	} else {
		ctx->result = (uint32_t)CRC_Get16bitResult(config->base);
	}

	ctx->state = CRC_STATE_IDLE;
	crc_nxp_unlock(dev);
	return 0;
}

static DEVICE_API(crc, crc_nxp_driver_api) = {
	.begin = crc_nxp_begin,
	.update = crc_nxp_update,
	.finish = crc_nxp_finish,
};

static int crc_nxp_init(const struct device *dev)
{
	const struct crc_nxp_config *config = dev->config;
	struct crc_nxp_data *data = dev->data;

	if (config->clock_dev != NULL) {
		if (!device_is_ready(config->clock_dev)) {
			return -ENODEV;
		}

		int ret = clock_control_on(config->clock_dev, config->clock_subsys);

		if (ret != 0) {
			return ret;
		}
	}

	if (config->reset.dev != NULL) {
		if (!device_is_ready(config->reset.dev)) {
			return -ENODEV;
		}

		int ret = reset_line_deassert_dt(&config->reset);

		if (ret != 0) {
			return ret;
		}
	}

	k_sem_init(&data->lock, 1, 1);
	return 0;
}

#define CRC_NXP_INIT(inst)                                                                         \
	static struct crc_nxp_data crc_nxp_data_##inst;                                            \
	static const struct crc_nxp_config crc_nxp_config_##inst = {                               \
		.base = (CRC_Type *)DT_INST_REG_ADDR(inst),                                        \
		.clock_dev = COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, clocks),                      \
			(DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst))), (NULL)),                       \
		.clock_subsys = COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, clocks),                   \
			((clock_control_subsys_t)DT_INST_CLOCKS_CELL(inst, name)),                 \
			((clock_control_subsys_t)0U)),                                             \
		.reset = RESET_DT_SPEC_INST_GET_OR(inst, {0}),                                     \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, crc_nxp_init, NULL, &crc_nxp_data_##inst,                      \
			      &crc_nxp_config_##inst, POST_KERNEL,                                 \
			      CONFIG_CRC_DRIVER_INIT_PRIORITY, &crc_nxp_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CRC_NXP_INIT)
