/*
 * Copyright (c) 2024 sensry.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sensry_sy1xx_spi

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_sy1xx);

#include "spi_context.h"
#include <errno.h>
#include <zephyr/spinlock.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <udma.h>
#include <zephyr/drivers/pinctrl.h>
#include "zephyr/sys/byteorder.h"

/* SPI udma command interface definitions */
#define SPI_CMD_OFFSET (4)

/* Commands for SPI UDMA */
#define SPI_CMD_CFG       (0 << SPI_CMD_OFFSET)
#define SPI_CMD_SOT       (1 << SPI_CMD_OFFSET)
#define SPI_CMD_EOT       (9 << SPI_CMD_OFFSET)
#define SPI_CMD_FULL_DPLX (12 << SPI_CMD_OFFSET)

/* CMD CFG */
#define SPI_CMD_CFG_0()           (SPI_CMD_CFG)
#define SPI_CMD_CFG_1()           (0)
#define SPI_CMD_CFG_2(cpol, cpha) (((cpol & 0x1) << 1) | ((cpha & 0x1) << 0))
#define SPI_CMD_CFG_3(div)        (div & 0xff)

/* CMD SOT */
#define SPI_CMD_SOT_0()   (SPI_CMD_SOT)
#define SPI_CMD_SOT_1()   (0)
#define SPI_CMD_SOT_2()   (0)
#define SPI_CMD_SOT_3(cs) (cs & 0x1)

/* CMD FULL_DPLX */
#define SPI_CMD_FULL_DPLX_DATA0(align) (SPI_CMD_FULL_DPLX | ((align & 0x3) << 1))
#define SPI_CMD_FULL_DPLX_DATA1()      (0)

/* CMD EOT */
#define SPI_CMD_EOT0()    (SPI_CMD_EOT)
#define SPI_CMD_EOT1()    (0)
#define SPI_CMD_EOT2()    (0)
#define SPI_CMD_EOT3(evt) (evt & 0x1)

#define SY1XX_SPI_WORD_SIZE_8_BIT (8)
#define SY1XX_SPI_WORD_ALIGN      (0)

#define SY1XX_SPI_MAX_CTRL_BYTE_SIZE (4)
#define SY1XX_SPI_MIN_BUFFER_SIZE    (SY1XX_SPI_MAX_CTRL_BYTE_SIZE + 1)

BUILD_ASSERT(CONFIG_SPI_SY1XX_BUFFER_SIZE >= SY1XX_SPI_MIN_BUFFER_SIZE,
	     "CONFIG_SPI_SY1XX_BUFFER_SIZE too small for control bytes");

BUILD_ASSERT((CONFIG_SPI_SY1XX_BUFFER_SIZE % 4) == 0,
	     "CONFIG_SPI_SY1XX_BUFFER_SIZE must be 4-byte aligned");

BUILD_ASSERT((CONFIG_SPI_SY1XX_BUFFER_SIZE - SY1XX_SPI_MAX_CTRL_BYTE_SIZE) <=
		     (0x10000 / SY1XX_SPI_WORD_SIZE_8_BIT),
	     "CONFIG_SPI_SY1XX_BUFFER_SIZE too large for transfer");

struct sy1xx_spi_config {
	/* dma base address */
	uint32_t base;
	/* pin ctrl for all spi related pins */
	const struct pinctrl_dev_config *pcfg;
	/* number of instance, spi0, spi1, ... */
	uint32_t inst;
	/* character used to fill tx fifo, while reading (default: 0xff) */
	uint8_t overrun_char;
};

/* place buffer in dma section */
struct sy1xx_spi_dma_buffer {
	uint8_t write[CONFIG_SPI_SY1XX_BUFFER_SIZE];
	uint8_t read[CONFIG_SPI_SY1XX_BUFFER_SIZE];
};

struct sy1xx_spi_data {
	struct spi_context ctx;
	/* pin only used, if not using gpio and explicitly selected in dt */
	int32_t cs_pin;
	/* reference to dma buffers */
	struct sy1xx_spi_dma_buffer *dma;
	/* current bus configuration */
	uint8_t cpol;
	uint8_t cpha;
	uint8_t divider;
};

static int sy1xx_spi_release(const struct device *dev, const struct spi_config *config);

static int sy1xx_spi_init(const struct device *dev)
{
	const struct sy1xx_spi_config *cfg = dev->config;
	struct sy1xx_spi_data *data = dev->data;
	int32_t ret;

	/* UDMA clock enable */
	sy1xx_udma_enable_clock(SY1XX_UDMA_MODULE_SPI, cfg->inst);

	/* PAD config */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("SPI failed to set pin config for %u", cfg->inst);
		return ret;
	}

	/* reset udma */
	SY1XX_UDMA_CANCEL_RX(cfg->base);
	SY1XX_UDMA_CANCEL_TX(cfg->base);

	SY1XX_UDMA_WAIT_FOR_FINISHED_TX(cfg->base);
	SY1XX_UDMA_WAIT_FOR_FINISHED_RX(cfg->base);

	/* prepare context for cs */
	ret = spi_context_cs_configure_all(&data->ctx);
	if (ret < 0) {
		LOG_ERR("SPI failed to configure");
		return ret;
	}

	/* reset config, expected to come with first transfer */
	data->ctx.config = NULL;

	return sy1xx_spi_release(dev, NULL);
}

static int sy1xx_spi_update_cfg(const struct device *dev)
{
	const struct sy1xx_spi_config *cfg = dev->config;
	struct sy1xx_spi_data *data = dev->data;
	uint8_t *buf;

	buf = &data->dma->write[0];

	/* prepare bus cfg */
	buf[0] = SPI_CMD_CFG_3(data->divider);
	buf[1] = SPI_CMD_CFG_2(data->cpol, data->cpha);
	buf[2] = SPI_CMD_CFG_1();
	buf[3] = SPI_CMD_CFG_0();

	/* transfer configuration via udma to spi controller */
	SY1XX_UDMA_START_TX(cfg->base, (uintptr_t)buf, 4, 0);
	SY1XX_UDMA_WAIT_FOR_FINISHED_TX(cfg->base);

	if (SY1XX_UDMA_GET_REMAINING_TX(cfg->base)) {
		return -EIO;
	}

	return 0;
}

static int sy1xx_spi_configure(const struct device *dev, const struct spi_config *config)
{
	struct sy1xx_spi_data *data = dev->data;
	uint32_t divider;
	const uint32_t unsupported = SPI_TRANSFER_LSB | SPI_MODE_LOOP | SPI_HALF_DUPLEX;

	if (spi_context_configured(&data->ctx, config)) {
		return 0;
	}

	if (!spi_cs_is_gpio(config)) {
		/* if cs0 and cs1 are not real gpios, that indicates, we are using the hw cs */
		if (config->slave > 1U) {
			return -EINVAL;
		}
		data->cs_pin = (int32_t)config->slave;
	} else {
		data->cs_pin = -1;
	}

	if (SPI_OP_MODE_GET(config->operation) != SPI_OP_MODE_MASTER) {
		/* Slave mode is not implemented. */
		return -ENOTSUP;
	}

	if ((config->operation & unsupported) != 0U) {
		return -ENOTSUP;
	}

	if (SPI_WORD_SIZE_GET(config->operation) != SY1XX_SPI_WORD_SIZE_8_BIT) {
		/* only 8 bit mode is implemented. */
		return -ENOTSUP;
	}

	if ((config->operation & SPI_MODE_CPOL) == 0U) {
		data->cpol = 0;
	} else {
		data->cpol = 1;
	}

	if ((config->operation & SPI_MODE_CPHA) == 0U) {
		data->cpha = 0;
	} else {
		data->cpha = 1;
	}

	/* peripheral fixed pre-scaler 1:2 */
	divider = DIV_ROUND_UP(sy1xx_soc_get_peripheral_clock(), 2 * config->frequency);
	if (!IN_RANGE(divider, 1U, 0xFFU)) {
		return -EINVAL;
	}

	data->divider = (uint8_t)divider;

	data->ctx.config = config;

	return sy1xx_spi_update_cfg(dev);
}

static int sy1xx_spi_set_cs(const struct device *dev)
{
	const struct sy1xx_spi_config *cfg = dev->config;
	struct sy1xx_spi_data *data = dev->data;

	/* check if spi controller chip select is configured */
	if (data->cs_pin >= 0) {
		uint8_t *buf = &data->dma->write[0];

		/* start with selecting hardware chip-select */
		buf[0] = SPI_CMD_SOT_3(data->cs_pin);
		buf[1] = SPI_CMD_SOT_2();
		buf[2] = SPI_CMD_SOT_1();
		buf[3] = SPI_CMD_SOT_0();

		/* transfer configuration via udma to spi controller */
		SY1XX_UDMA_START_TX(cfg->base, (uintptr_t)buf, 4, 0);
		SY1XX_UDMA_WAIT_FOR_FINISHED_TX(cfg->base);

		if (SY1XX_UDMA_GET_REMAINING_TX(cfg->base)) {
			return -EIO;
		}
	}

	/* enable gpio cs (if configured) */
	spi_context_cs_control(&data->ctx, true);

	return 0;
}

static int32_t sy1xx_spi_reset_cs(const struct device *dev)
{
	const struct sy1xx_spi_config *cfg = dev->config;
	struct sy1xx_spi_data *data = dev->data;

	if (data->cs_pin >= 0) {
		uint8_t *buf = &data->dma->write[0];

		/* reset the chip select of controller */
		buf[0] = SPI_CMD_EOT3(0);
		buf[1] = SPI_CMD_EOT2();
		buf[2] = SPI_CMD_EOT1();
		buf[3] = SPI_CMD_EOT0();

		SY1XX_UDMA_START_TX(cfg->base, (uintptr_t)buf, 4, 0);
		SY1XX_UDMA_WAIT_FOR_FINISHED_TX(cfg->base);

		if (SY1XX_UDMA_GET_REMAINING_TX(cfg->base)) {
			return -EIO;
		}
	}

	/* reset gpio chip select */
	spi_context_cs_control(&data->ctx, false);

	return 0;
}

/*
 * transfer the next available chunk from the context
 * - we cut the rx/tx buffers into chunks that:
 *   - we have symmetric tx and rx length
 *   - we have half-duplex where tx or rx length is 0
 * - note: tx_len > 0 or rx_len > 0 while tx_buf or rx_buf is NULL is used to indicate empty
 *         transfers on either rx or tx
 */
static int32_t sy1xx_spi_full_duplex_transfer_ctx(const struct device *dev)
{
	const struct sy1xx_spi_config *cfg = dev->config;
	struct sy1xx_spi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint8_t *buf;
	uint32_t padded_len;
	uint32_t xfer_len;

	/* calculate symmetric or half-duplex lengths for the next transfer */
	xfer_len = spi_context_max_continuous_chunk(ctx);
	xfer_len = min(xfer_len, CONFIG_SPI_SY1XX_BUFFER_SIZE - SY1XX_SPI_MAX_CTRL_BYTE_SIZE);

	buf = &data->dma->write[0];

	/* expected data config (bit-len in bits) and spi transfer type */
	sys_put_le16(xfer_len * SY1XX_SPI_WORD_SIZE_8_BIT, &buf[0]);
	buf[2] = SPI_CMD_FULL_DPLX_DATA1();
	buf[3] = SPI_CMD_FULL_DPLX_DATA0(SY1XX_SPI_WORD_ALIGN);

	/* data; we need to fill multiple of 32bit size */
	padded_len = ROUND_UP(xfer_len, 4);

	if (spi_context_tx_buf_on(ctx)) {
		memcpy(&buf[SY1XX_SPI_MAX_CTRL_BYTE_SIZE], ctx->tx_buf, xfer_len);
	} else {
		memset(&buf[SY1XX_SPI_MAX_CTRL_BYTE_SIZE], cfg->overrun_char, xfer_len);
	}

	/* always execute a full-duplex transfer from/to the internal DMA buffer */
	SY1XX_UDMA_START_RX(cfg->base, (uintptr_t)data->dma->read, xfer_len,
			    SY1XX_UDMA_RX_DATA_ADDR_INC_SIZE_32);
	SY1XX_UDMA_START_TX(cfg->base, (uintptr_t)data->dma->write,
			    SY1XX_SPI_MAX_CTRL_BYTE_SIZE + padded_len, 0);

	SY1XX_UDMA_WAIT_FOR_FINISHED_TX(cfg->base);
	SY1XX_UDMA_WAIT_FOR_FINISHED_RX(cfg->base);

	if (SY1XX_UDMA_GET_REMAINING_TX(cfg->base)) {
		LOG_ERR("not all bytes sent");
		return -EIO;
	}
	if (SY1XX_UDMA_GET_REMAINING_RX(cfg->base)) {
		LOG_ERR("not all bytes received");
		return -EIO;
	}

	/* transfer from dma buffer to the rx_buf, if requested */
	if (spi_context_rx_buf_on(ctx)) {
		memcpy(ctx->rx_buf, &data->dma->read[0], xfer_len);
	}

	/* report the actual transferred number of bytes and switch to next buf-set/chunk */
	spi_context_update_rx(ctx, 1, xfer_len);
	spi_context_update_tx(ctx, 1, xfer_len);

	return 0;
}

static int sy1xx_spi_transceive_sync(const struct device *dev, const struct spi_config *config,
				     const struct spi_buf_set *tx_bufs,
				     const struct spi_buf_set *rx_bufs)
{
	struct sy1xx_spi_data *data = dev->data;
	struct spi_context *ctx;
	int ret = 0;

	spi_context_lock(&data->ctx, false, NULL, NULL, config);

	ret = sy1xx_spi_configure(dev, config);
	if (ret < 0) {
		goto done;
	}

	ret = sy1xx_spi_set_cs(dev);
	if (ret < 0) {
		LOG_ERR("SPI set cs failed");
		goto done;
	}

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

	ctx = &data->ctx;
	while (spi_context_tx_on(ctx) || spi_context_rx_on(ctx)) {
		ret = sy1xx_spi_full_duplex_transfer_ctx(dev);
		if (ret < 0) {
			LOG_ERR("Failed to transfer data - %d", ret);
			goto done;
		}
	}

done:
	sy1xx_spi_reset_cs(dev);
	spi_context_release(&data->ctx, ret);

	return ret;
}

static int sy1xx_spi_release(const struct device *dev, const struct spi_config *config)
{
	struct sy1xx_spi_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);
	return 0;
}

static DEVICE_API(spi, sy1xx_spi_driver_api) = {
	.transceive = sy1xx_spi_transceive_sync,
	.release = sy1xx_spi_release,
};

#define SPI_SY1XX_DEVICE_INIT(n)                                                                   \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static const struct sy1xx_spi_config sy1xx_spi_dev_config_##n = {                          \
		.base = (uint32_t)DT_INST_REG_ADDR(n),                                             \
		.inst = (uint32_t)DT_INST_PROP(n, instance),                                       \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.overrun_char = DT_INST_PROP_OR(n, overrun_character, 0xff),                       \
	};                                                                                         \
                                                                                                   \
	static struct sy1xx_spi_dma_buffer __attribute__((section(".udma_access")))                \
	__aligned(4) sy1xx_spi_dev_dma_##n = {};                                                   \
                                                                                                   \
	static struct sy1xx_spi_data sy1xx_spi_dev_data_##n = {                                    \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)                               \
			SPI_CONTEXT_INIT_LOCK(sy1xx_spi_dev_data_##n, ctx),                        \
		SPI_CONTEXT_INIT_SYNC(sy1xx_spi_dev_data_##n, ctx),                                \
		.dma = &sy1xx_spi_dev_dma_##n,                                                     \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &sy1xx_spi_init, NULL, &sy1xx_spi_dev_data_##n,                   \
			      &sy1xx_spi_dev_config_##n, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,    \
			      &sy1xx_spi_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_SY1XX_DEVICE_INIT)
