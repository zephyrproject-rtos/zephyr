/*
 * Copyright (c) 2026 Texas Instruments Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#define DT_DRV_COMPAT ti_mspm0_crc

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/crc.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(crc_ti_mspm0, CONFIG_CRC_LOG_LEVEL);

#define CRC_MSPM0_PWREN_MASK			BIT(0)
#define CRC_MSPM0_PWREN_KEY_MASK		GENMASK(31, 24)
#define CRC_MSPM0_PWREN_KEY			FIELD_PREP(CRC_MSPM0_PWREN_KEY_MASK, 0x26)

#define CRC_MSPM0_CLKSEL_MCLK_SEL		BIT(0)

#define CRC_MSPM0_CRCCTRL_POLYSIZE		BIT(0)
#define CRC_MSPM0_CRCCTRL_BITREVERSE		BIT(1)
#define CRC_MSPM0_CRCCTRL_INPUT_ENDIANNESS	BIT(2)
#define CRC_MSPM0_CRCCTRL_OUTPUT_BYTESWAP	BIT(4)

typedef struct {
	uint32_t reserved0[0x200];
	volatile uint32_t pwren;		/* Power Enable			@0x800h */
	volatile uint32_t rstctl;		/* Reset Control		@0x804h */
	uint32_t reserved1[3];
	volatile uint32_t stat;			/* Status Register		@0x814h */
	uint32_t reserved2[0x1FB];
	volatile uint32_t clksel;		/* Clock Select			@0x1004h */
	uint32_t reserved3[0x3D];
	volatile uint32_t desc;			/* Module Description		@0x10FCh */
	volatile uint32_t crcctrl;		/* Control Register		@0x1100h */
	volatile uint32_t crcseed;		/* Seed Register		@0x1104h */
	volatile uint32_t crcin;		/* Input Data Register		@0x1108h */
	volatile uint32_t crcout;		/* Output Result Register	@0x110Ch */
	volatile uint32_t crcpoly;		/* Polynomial Config Register	@0x1110h */
	uint32_t reserved4[0x1BB];
	volatile uint32_t crcin_idx[0x200];	/* Input Data Array		@0x1800h */
} crc_ti_mspm0_reg_t;

struct crc_mspm0_config {
	crc_ti_mspm0_reg_t *regs;
};

struct crc_ti_mspm0_data {
	struct k_sem sem_lock;
	struct k_mutex update_lock;
};

static int crc_mspm0_hw_init(const struct crc_mspm0_config *config, struct crc_ctx *ctx)
{
	switch (ctx->type) {
	case CRC16_CCITT:
		config->regs->crcctrl = CRC_MSPM0_CRCCTRL_POLYSIZE;
		break;
	case CRC32_IEEE:
		config->regs->crcctrl = 0;
		break;
	default:
		LOG_ERR("unsupported CRC type %d", ctx->type);
		return -EINVAL;
	}

	config->regs->crcpoly = ctx->polynomial;

	/*
	 * BITREVERSE controls both input and output bit reversal simultaneously.
	 * Set it when input reversal is requested; output reversal is handled
	 * the same way since the hardware cannot separate the two.
	 */
	if (ctx->reversed & CRC_FLAG_REVERSE_INPUT) {
		config->regs->crcctrl |= CRC_MSPM0_CRCCTRL_BITREVERSE;
	}

	config->regs->crcseed = ctx->seed;

	return 0;
}

static int crc_mspm0_begin(const struct device *dev, struct crc_ctx *ctx)
{
	const struct crc_mspm0_config *config = dev->config;
	struct crc_ti_mspm0_data *data = dev->data;
	int ret;

	if (ctx == NULL) {
		LOG_ERR("Null context passed.");
		return -EINVAL;
	}

	if (k_sem_take(&data->sem_lock, K_NO_WAIT) != 0) {
		LOG_ERR("CRC already in progress");
		return -EBUSY;
	}

	ret = crc_mspm0_hw_init(config, ctx);

	if (ret != 0) {
		k_sem_give(&data->sem_lock);
		return ret;
	}

	ctx->state = CRC_STATE_IN_PROGRESS;

	return 0;
}

static int crc_mspm0_update(const struct device *dev, struct crc_ctx *ctx, const void *buffer,
			  size_t bufsize)
{
	const struct crc_mspm0_config *config = dev->config;
	struct crc_ti_mspm0_data *data = dev->data;
	const uint8_t *buf = buffer;

	if (buf == NULL && bufsize > 0U) {
		LOG_ERR("NULL buffer with non-zero size");
		ctx->state = CRC_STATE_IDLE;
		k_sem_give(&data->sem_lock);
		return -EINVAL;
	}

	k_mutex_lock(&data->update_lock, K_FOREVER);
	while (bufsize > 0U) {
		size_t chunk = MIN(bufsize, sizeof(config->regs->crcin_idx));

		memcpy((void *)config->regs->crcin_idx, buf, chunk);
		buf += chunk;
		bufsize -= chunk;
	}
	k_mutex_unlock(&data->update_lock);

	return 0;
}

static int crc_mspm0_finish(const struct device *dev, struct crc_ctx *ctx)
{
	const struct crc_mspm0_config *config = dev->config;
	struct crc_ti_mspm0_data *data = dev->data;

	/* Only configure output reversal if it selected as well */
	if (ctx->reversed & CRC_FLAG_REVERSE_OUTPUT) {
		config->regs->crcctrl |= CRC_MSPM0_CRCCTRL_BITREVERSE;
	} else {
		config->regs->crcctrl &= ~(CRC_MSPM0_CRCCTRL_BITREVERSE);
	}

	/* flip the bits as required by IEEE standard */
	if (ctx->type == CRC32_IEEE) {
		ctx->result = (config->regs->crcout ^ 0xFFFFFFFF);
	} else {
		ctx->result = config->regs->crcout;
	}

	ctx->state = CRC_STATE_IDLE;
	k_sem_give(&data->sem_lock);

	return 0;
}

static int crc_mspm0_init(const struct device *dev)
{
	const struct crc_mspm0_config *config = dev->config;
	struct crc_ti_mspm0_data *data = dev->data;

	k_sem_init(&data->sem_lock, 1, 1);
	k_mutex_init(&data->update_lock);

	if (!(config->regs->pwren & CRC_MSPM0_PWREN_MASK)) {
		config->regs->pwren = CRC_MSPM0_PWREN_KEY | CRC_MSPM0_PWREN_MASK;
	}

	LOG_DBG("CRC peripheral initialized");

	return 0;
}

static DEVICE_API(crc, crc_mspm0_driver_api) = {
	.begin = crc_mspm0_begin,
	.update = crc_mspm0_update,
	.finish = crc_mspm0_finish,
};

#define CRC_TI_MSPM0_INIT(n)							\
	static const struct crc_mspm0_config crc_config_##n = {			\
		.regs = (crc_ti_mspm0_reg_t *)DT_INST_REG_ADDR(n),		\
	};									\
										\
	static struct crc_ti_mspm0_data crc_data_##n;				\
										\
	DEVICE_DT_INST_DEFINE(n, crc_mspm0_init, NULL,				\
			      &crc_data_##n, &crc_config_##n,			\
			      POST_KERNEL, CONFIG_CRC_DRIVER_INIT_PRIORITY,	\
			      &crc_mspm0_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CRC_TI_MSPM0_INIT)
