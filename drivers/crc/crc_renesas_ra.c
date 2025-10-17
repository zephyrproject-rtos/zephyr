/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(renesas_ra_crc, CONFIG_CRC_LOG_LEVEL);

#include <errno.h>
#include <zephyr/drivers/crc.h>

#include <soc.h>
#include "r_crc.h"
#include "rp_crc.h"

#define DT_DRV_COMPAT      renesas_ra_crc
#define DEFAULT_NUM_BYTES  (4U)
#define DEFAULT_SEED_VALUE (0x00000000)

struct crc_renesas_ra_cfg {
	const struct device *clock_dev;
	const struct clock_control_ra_subsys_cfg clock_id;
};

struct crc_renesas_ra_data {
	struct st_crc_instance_ctrl ctrl;
	struct st_crc_input_t input_data;
	struct st_crc_cfg crc_config;
	struct k_sem sem;
	bool flag_crc_updated;
};

static void crc_lock(const struct device *dev)
{
	struct crc_renesas_ra_data *data = dev->data;

	k_sem_take(&data->sem, K_FOREVER);
}

static void crc_unlock(const struct device *dev)
{
	struct crc_renesas_ra_data *data = dev->data;

	k_sem_give(&data->sem);
}

static int crc_set_config(const struct device *dev, struct crc_ctx *ctx)
{
	fsp_err_t err;
	struct crc_renesas_ra_data *data = dev->data;
	crc_cfg_t const *const crc_cfg = &data->crc_config;

	data->crc_config.bit_order = (ctx->reversed & CRC_FLAG_REVERSE_OUTPUT)
					     ? CRC_BIT_ORDER_LMS_LSB
					     : CRC_BIT_ORDER_LMS_MSB;

	switch (ctx->type) {
	case CRC8: {
		if ((ctx->polynomial != CRC8_POLY) && (ctx->polynomial != CRC8_REFLECT_POLY)) {
			return -EINVAL;
		}

		data->crc_config.polynomial = CRC_POLYNOMIAL_CRC_8;
		break;
	}
	case CRC16: {
		if (ctx->polynomial != CRC16_POLY) {
			return -EINVAL;
		}

		data->crc_config.polynomial = CRC_POLYNOMIAL_CRC_16;
		break;
	}
	case CRC16_CCITT: {
		if (ctx->polynomial != CRC16_CCITT_POLY) {
			return -EINVAL;
		}

		data->crc_config.polynomial = CRC_POLYNOMIAL_CRC_CCITT;
		break;
	}
	case CRC32_C: {
		if (ctx->polynomial != CRC32C_POLY) {
			return -EINVAL;
		}

		data->crc_config.polynomial = CRC_POLYNOMIAL_CRC_32C;
		break;
	}
	case CRC32_IEEE: {
		if (ctx->polynomial != CRC32_IEEE_POLY) {
			return -EINVAL;
		}

		data->crc_config.polynomial = CRC_POLYNOMIAL_CRC_32;
		break;
	}
	default:
		return -ENOTSUP;
	}

	err = RP_CRC_Reconfigure(&data->ctrl, crc_cfg);

	if (err != FSP_SUCCESS) {
		ctx->state = CRC_STATE_IDLE;
		crc_unlock(dev);
		return -EINVAL;
	}

	return 0;
}

static int crc_renesas_ra_begin(const struct device *dev, struct crc_ctx *ctx)
{
	int ret;

	crc_lock(dev);

	ret = crc_set_config(dev, ctx);
	if (ret != 0) {
		crc_unlock(dev);
		return ret;
	}

	ctx->state = CRC_STATE_IN_PROGRESS;

	return 0;
}

static int crc_renesas_ra_update(const struct device *dev, struct crc_ctx *ctx, const void *buffer,
				 size_t bufsize)
{
	struct crc_renesas_ra_data *data = dev->data;
	fsp_err_t err;
	uint32_t init_val;

	/* Ensure CRC calculation has been initialized by crc_begin() */
	if (ctx->state == CRC_STATE_IDLE) {
		return -EINVAL;
	}

	switch (ctx->type) {
	case CRC8:
		__fallthrough;
	case CRC16:
		__fallthrough;
	case CRC16_CCITT: {
		if (ctx->type == CRC8) {
			init_val = (data->flag_crc_updated ? ctx->result : ctx->seed) & 0xFF;
		} else {
			init_val = (data->flag_crc_updated ? ctx->result : ctx->seed) & 0xFFFF;
		}

		data->input_data.num_bytes = bufsize;
		data->input_data.crc_seed = init_val;
		data->input_data.p_input_buffer = (uint8_t *)buffer;

		err = R_CRC_Calculate(&data->ctrl, &data->input_data, &ctx->result);
		if (err != FSP_SUCCESS) {
			ctx->state = CRC_STATE_IDLE;
			crc_unlock(dev);
			return -EINVAL;
		}
		break;
	}
	default: {
		init_val = (data->flag_crc_updated ? ctx->result : ctx->seed) & 0xFFFFFFFF;

		if ((bufsize % 4) != 0) {
			ctx->state = CRC_STATE_IDLE;
			crc_unlock(dev);
			return -ENOTSUP;
		}

		data->input_data.num_bytes = bufsize;
		data->input_data.crc_seed = init_val;
		data->input_data.p_input_buffer = (uint8_t *)buffer;

		err = R_CRC_Calculate(&data->ctrl, &data->input_data, &ctx->result);
		if (err != FSP_SUCCESS) {
			ctx->state = CRC_STATE_IDLE;
			crc_unlock(dev);
			return -EINVAL;
		}

		if (ctx->type == CRC32_IEEE) {
			ctx->result = (crc_result_t)~ctx->result;
		}
		break;
	}
	}

	data->flag_crc_updated = true;

	return 0;
}

static int crc_renesas_ra_finish(const struct device *dev, struct crc_ctx *ctx)
{
	struct crc_renesas_ra_data *data = dev->data;

	if (ctx->state == CRC_STATE_IDLE) {
		return -EINVAL;
	}

	ctx->state = CRC_STATE_IDLE;

	data->flag_crc_updated = false;

	crc_unlock(dev);

	return 0;
}

static int crc_ra_init(const struct device *dev)
{
	int ret;
	fsp_err_t err;
	const struct crc_renesas_ra_cfg *cfg = dev->config;
	struct crc_renesas_ra_data *data = dev->data;
	crc_cfg_t const *const crc_cfg = &data->crc_config;

	if (!device_is_ready(cfg->clock_dev)) {
		LOG_ERR("CRC: Clock control device not ready");
		return -ENODEV;
	}

	ret = clock_control_on(cfg->clock_dev, (clock_control_subsys_t)&cfg->clock_id);
	if (ret < 0) {
		LOG_ERR("CRC: Clock control device could not initialize");
		return -EIO;
	}

	err = R_CRC_Open(&data->ctrl, crc_cfg);
	if (err != FSP_SUCCESS) {
		return -EINVAL;
	}

	k_sem_init(&data->sem, 1, 1);

	return 0;
}

static DEVICE_API(crc, crc_renesas_ra_driver_api) = {
	.begin = crc_renesas_ra_begin,
	.update = crc_renesas_ra_update,
	.finish = crc_renesas_ra_finish,
};

#define CRC_RA_INIT_CFG(idx)                                                                       \
	static const struct crc_renesas_ra_cfg crc_renesas_ra_cfg_##idx = {                        \
		.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_DRV_INST(idx))),                      \
		.clock_id =                                                                        \
			{                                                                          \
				.mstp = DT_CLOCKS_CELL_BY_IDX(DT_DRV_INST(idx), 0, mstp),          \
				.stop_bit = DT_CLOCKS_CELL_BY_IDX(DT_DRV_INST(idx), 0, stop_bit),  \
			},                                                                         \
	};

#define CRC_RA_INIT(idx)                                                                           \
	CRC_RA_INIT_CFG(idx);                                                                      \
                                                                                                   \
	static struct crc_renesas_ra_data crc_renesas_ra_data_##idx = {                            \
		.input_data =                                                                      \
			{                                                                          \
				.num_bytes = DEFAULT_NUM_BYTES,                                    \
				.crc_seed = DEFAULT_SEED_VALUE,                                    \
				.p_input_buffer = NULL,                                            \
			},                                                                         \
		.crc_config =                                                                      \
			{                                                                          \
				.bit_order = CRC_BIT_ORDER_LMS_LSB,                                \
				.polynomial = CRC_POLYNOMIAL_CRC_32,                               \
			},                                                                         \
		.flag_crc_updated = false,                                                         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, crc_ra_init, NULL, &crc_renesas_ra_data_##idx,                  \
			      &crc_renesas_ra_cfg_##idx, POST_KERNEL,                              \
			      CONFIG_CRC_DRIVER_INIT_PRIORITY, &crc_renesas_ra_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CRC_RA_INIT)
