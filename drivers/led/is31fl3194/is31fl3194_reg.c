/*
 * Copyright (c) 2022 Benjamin Björnsson <benjamin.bjornsson@gmail.com>.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "is31fl3194_reg.h"

#include <errno.h>
#include <stdbool.h>
#include <string.h>

static const struct is31fl3194_ops_conf ops_conf_default = {.ssd = 0b0, .rgb = 0b00, .out = 0b000};

static const struct is31fl3194_out_en out_en_default = {.en = 0b111};

static const struct is31fl3194_cbx cbx_default = {.cb1 = 0b01, .cb2 = 0b01, .cb3 = 0b01};

static const struct is31fl3194_outx_cl outx_cl_default = {.cl = 0b00000000};

static const struct is31fl3194_regs regs_default = {
	.ops_conf = ops_conf_default,
	.out_en = out_en_default,
	.cbx = cbx_default,
	.outx_cl = {outx_cl_default},
};

static uint8_t outx_cl_reg_addr[] = {IS31FL3194_OUT1_CURRENT_LEVEL, IS31FL3194_OUT2_CURRENT_LEVEL,
				     IS31FL3194_OUT3_CURRENT_LEVEL};

static int read_reg(const struct is31fl3194_ctx *ctx, uint8_t reg, uint8_t *data, uint16_t len)
{
	return ctx->read_reg(ctx->handle, reg, data, len);
}

static int write_reg(const struct is31fl3194_ctx *ctx, uint8_t reg, uint8_t *data, uint16_t len)
{
	return ctx->write_reg(ctx->handle, reg, data, len);
}

static bool valid_output(enum is31fl3194_out out)
{
	return (out == IS31FL3194_OUT1) || (out == IS31FL3194_OUT2) || (out == IS31FL3194_OUT3);
}

int is31fl3194_product_id_get(const struct is31fl3194_ctx *ctx, uint8_t *val)
{
	return read_reg(ctx, IS31FL3194_PRODUCT_ID, val, 1);
}

int is31fl3194_ops_ssd_set(const struct is31fl3194_ctx *ctx, enum is31fl3194_ssd val)
{
	ctx->regs->ops_conf.ssd = val;

	return write_reg(ctx, IS31FL3194_OPERATING_CONF, (uint8_t *)&ctx->regs->ops_conf, 1);
}

void is31fl3194_ops_ssd_get(const struct is31fl3194_ctx *ctx, enum is31fl3194_ssd *val)
{
	*val = ctx->regs->ops_conf.ssd;
}

#define SET_BIT_N(byte, value, n) ((byte) = ((byte) & ~(0x1U << (n))) | (value * (0x1U << (n))))

int is31fl3194_out_en_set(const struct is31fl3194_ctx *ctx, enum is31fl3194_out out,
			  enum is31fl3194_outx_en val)
{
	if (!valid_output(out)) {
		return -EINVAL;
	}

	SET_BIT_N(ctx->regs->out_en.en, val, out);

	return write_reg(ctx, IS31FL3194_OUTPUT_ENABLE, (uint8_t *)&ctx->regs->out_en, 1);
}

#define BIT_VALUE_N(byte, n) (((byte) & (0x1U << n)) >> n)

int is31fl3194_out_en_get(const struct is31fl3194_ctx *ctx, enum is31fl3194_out out,
			  enum is31fl3194_outx_en *val)
{
	if (!valid_output(out)) {
		return -EINVAL;
	}

	*val = BIT_VALUE_N(ctx->regs->out_en.en, out);

	return 0;
}

int is31fl3194_current_band_set(const struct is31fl3194_ctx *ctx, enum is31fl3194_out out,
				enum is31fl3194_current_band val)
{
	if (out == IS31FL3194_OUT1) {
		ctx->regs->cbx.cb1 = val;
	} else if (out == IS31FL3194_OUT2) {
		ctx->regs->cbx.cb2 = val;
	} else if (out == IS31FL3194_OUT3) {
		ctx->regs->cbx.cb3 = val;
	} else {
		return -EINVAL;
	}

	return write_reg(ctx, IS31FL3194_CURRENT_BAND, (uint8_t *)&ctx->regs->cbx, 1);
}

int is31fl3194_current_band_get(const struct is31fl3194_ctx *ctx, enum is31fl3194_out out,
				enum is31fl3194_current_band *val)
{
	if (out == IS31FL3194_OUT1) {
		*val = ctx->regs->cbx.cb1;
	} else if (out == IS31FL3194_OUT2) {
		*val = ctx->regs->cbx.cb2;
	} else if (out == IS31FL3194_OUT3) {
		*val = ctx->regs->cbx.cb3;
	} else {
		return -EINVAL;
	}

	return 0;
}

int is31fl3194_outx_cl_set(const struct is31fl3194_ctx *ctx, enum is31fl3194_out out,
			   struct is31fl3194_outx_cl val)
{
	if (!valid_output(out)) {
		return -EINVAL;
	}

	ctx->regs->outx_cl[out] = val;

	return write_reg(ctx, outx_cl_reg_addr[out], (uint8_t *)&ctx->regs->outx_cl[out], 1);
}

int is31fl3194_outx_cl_get(const struct is31fl3194_ctx *ctx, enum is31fl3194_out out,
			   struct is31fl3194_outx_cl *val)
{
	if (!valid_output(out)) {
		return -EINVAL;
	}

	*val = ctx->regs->outx_cl[out];

	return 0;
}


int is31fl3194_color_update(const struct is31fl3194_ctx *ctx)
{
	union is31fl3194_reg reg = {.byte = IS31FL3194_UPDATE};

	return write_reg(ctx, IS31FL3194_COLOR_UPDATE, &reg.byte, 1);
}

int is31fl3194_reset(const struct is31fl3194_ctx *ctx)
{
	int res;
	union is31fl3194_reg reg = {.byte = IS31FL3194_UPDATE};

	res = write_reg(ctx, IS31FL3194_RESET, &reg.byte, 1);
	if (res) {
		return res;
	}

	memcpy(ctx->regs, &regs_default, sizeof(regs_default));

	return 0;
}
