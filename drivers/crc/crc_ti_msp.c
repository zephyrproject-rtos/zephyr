/*
 * Copyright (c) 2026 Texas Instruments Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#define DT_DRV_COMPAT ti_msp_crc

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/crc.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(crc_ti_msp, CONFIG_CRC_DRIVER_LOG_LEVEL);

#define CRC_MSP_PWREN_MASK			BIT(0)
#define CRC_MSP_PWREN_KEY_MASK			GENMASK(31, 24)
#define CRC_MSP_PWREN_KEY			FIELD_PREP(CRC_MSP_PWREN_KEY_MASK, 0x26)

#define CRC_MSP_CLKSEL_MCLK_SEL			BIT(0)

#define CRC_MSP_CRCCTRL_POLYSIZE		BIT(0)
#define CRC_MSP_CRCCTRL_BITREVERSE		BIT(1)
#define CRC_MSP_CRCCTRL_INPUT_ENDIANNESS	BIT(2)
#define CRC_MSP_CRCCTRL_OUTPUT_BYTESWAP		BIT(4)

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
} crc_ti_msp_reg_t;

struct crc_msp_config {
	crc_ti_msp_reg_t *regs;
};

struct crc_ti_msp_data {
	struct k_mutex mutex_lock;
	struct k_spinlock spin_lock;
};

static int crc_msp_hw_init(const struct crc_msp_config *config, struct crc_ctx *ctx)
{
	switch (ctx->type) {
	case CRC16_CCITT:
		config->regs->crcctrl = CRC_MSP_CRCCTRL_POLYSIZE;
		config->regs->crcpoly = CRC16_CCITT_POLY;
		break;
	case CRC32_IEEE:
		config->regs->crcctrl = 0;
		config->regs->crcpoly = CRC32_IEEE_POLY;
		break;
	default:
		LOG_ERR("unsupported CRC type %d", ctx->type);
		return -EINVAL;
	}

	/*
	 * BITREVERSE controls both input and output bit reversal simultaneously.
	 * Set it when input reversal is requested; output reversal is handled
	 * the same way since the hardware cannot separate the two.
	 */
	if (ctx->reversed & CRC_FLAG_REVERSE_INPUT) {
		config->regs->crcctrl |= CRC_MSP_CRCCTRL_BITREVERSE;
	}

	config->regs->crcseed = ctx->seed;

	return 0;
}

static int crc_msp_begin(const struct device *dev, struct crc_ctx *ctx)
{
	const struct crc_msp_config *config = dev->config;
	struct crc_ti_msp_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->mutex_lock, K_FOREVER);
	K_SPINLOCK(&data->spin_lock) {
		ret = crc_msp_hw_init(config, ctx);
		if (ret != 0) {
			K_SPINLOCK_BREAK;
		}
	}

	if (ret != 0) {
		k_mutex_unlock(&data->mutex_lock);
		return ret;
	}

	ctx->state = CRC_STATE_IN_PROGRESS;

	return 0;
}

static int crc_msp_update(const struct device *dev, struct crc_ctx *ctx, const void *buffer,
			  size_t bufsize)
{
	const struct crc_msp_config *config = dev->config;
	struct crc_ti_msp_data *data = dev->data;
	const uint8_t *buf = buffer;

	if (ctx->state != CRC_STATE_IN_PROGRESS) {
		LOG_ERR("crc_begin not called");
		return -EINVAL;
	}

	K_SPINLOCK(&data->spin_lock) {
		while (bufsize > 0U) {
			size_t chunk = MIN(bufsize, sizeof(config->regs->crcin_idx));

			memcpy((void *)config->regs->crcin_idx, buf, chunk);
			buf += chunk;
			bufsize -= chunk;
		}
	}

	return 0;
}

static int crc_msp_finish(const struct device *dev, struct crc_ctx *ctx)
{
	const struct crc_msp_config *config = dev->config;
	struct crc_ti_msp_data *data = dev->data;

	if (ctx->state != CRC_STATE_IN_PROGRESS) {
		LOG_ERR("crc_begin not called");
		return -EINVAL;
	}

	if (ctx->reversed & CRC_FLAG_REVERSE_OUTPUT) {
		config->regs->crcctrl |= CRC_MSP_CRCCTRL_BITREVERSE;
	}

	/* flip the bits as required by IEEE standard */
	if (ctx->type == CRC32_IEEE) {
		ctx->result = (config->regs->crcout ^ 0xFFFFFFFF);
	} else {
		ctx->result = config->regs->crcout;
	}

	ctx->state = CRC_STATE_IDLE;
	k_mutex_unlock(&data->mutex_lock);

	return 0;
}

static int crc_msp_init(const struct device *dev)
{
	const struct crc_msp_config *config = dev->config;
	struct crc_ti_msp_data *data = dev->data;

	k_mutex_init(&data->mutex_lock);

	if (!(config->regs->pwren & CRC_MSP_PWREN_MASK)) {
		config->regs->pwren = CRC_MSP_PWREN_KEY | CRC_MSP_PWREN_MASK;
	}

	k_busy_wait(k_cyc_to_us_ceil32(CONFIG_MSPM0_PERIPH_STARTUP_DELAY));

	/* Select clock source as mclk */
	config->regs->clksel = CRC_MSP_CLKSEL_MCLK_SEL;

	LOG_DBG("CRC peripheral initialized");

	return 0;
}

static DEVICE_API(crc, crc_msp_driver_api) = {
	.begin = crc_msp_begin,
	.update = crc_msp_update,
	.finish = crc_msp_finish,
};

#define CRC_TI_MSP_INIT(n)							\
	static const struct crc_msp_config crc_config_##n = {			\
		.regs = (crc_ti_msp_reg_t *)DT_INST_REG_ADDR(n),		\
	};									\
										\
	static struct crc_ti_msp_data crc_data_##n;				\
										\
	DEVICE_DT_INST_DEFINE(n, crc_msp_init, NULL,				\
			      &crc_data_##n, &crc_config_##n,			\
			      POST_KERNEL, CONFIG_CRC_DRIVER_INIT_PRIORITY,	\
			      &crc_msp_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CRC_TI_MSP_INIT)
