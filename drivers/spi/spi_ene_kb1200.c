/*
 * Copyright (c) 2024 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ene_kb1200_spi

#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>
#include <reg/spih.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_kb1200_spih, CONFIG_SPI_LOG_LEVEL);
#include "spi_context.h"

struct kb1200_spi_config {
	struct spih_regs *base_addr;
	const struct pinctrl_dev_config *pcfg;
};

struct kb1200_spi_data {
	struct spi_context ctx;
	uint8_t bytes_per_frame;
};

static int spi_kb1200_configure(const struct device *dev, const struct spi_config *spi_cfg)
{
	const struct kb1200_spi_config *config = dev->config;
	struct spih_regs *spih = config->base_addr;
	struct kb1200_spi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint8_t mode, clock_freq;
	uint8_t frame_size;

	if (spi_context_configured(&data->ctx, spi_cfg)) {
		/* This configuration is already in use */
		return 0;
	}

	if (SPI_OP_MODE_GET(spi_cfg->operation) == SPI_OP_MODE_SLAVE) {
		LOG_ERR("spih not support slave");
		return -ENOTSUP;
	}

	if (spi_cfg->operation & SPI_HALF_DUPLEX) {
		LOG_ERR("Half duplex mode is not supported");
		return -ENOTSUP;
	}

	if (spi_cfg->operation & SPI_MODE_LOOP) {
		LOG_ERR("Loopback mode is not supported");
		return -ENOTSUP;
	}

	if (spi_cfg->operation & SPI_TRANSFER_LSB) {
		LOG_ERR("spih not support transfer LSB");
		return -ENOTSUP;
	}

	if (((spi_cfg->operation) & (SPI_LINES_MASK)) != SPI_LINES_SINGLE) {
		LOG_ERR("spih not support dual/quad mode");
		return -ENOTSUP;
	}

	/*
	 * SPI signalling mode: CPOL and CPHA
	 * Mode CPOL CPHA
	 *  0     0    0
	 *  1     0    1
	 *  2     1    0
	 *  3     1    1
	 */
	mode = SPI_MODE_GET(spi_cfg->operation) & 0x03;

	/* Set the SPI frequency */
	if (spi_cfg->frequency < KHZ(500)) {
		LOG_ERR("Frequencies lower than 500kHz are not supported");
		return -ENOTSUP;
	} else if (spi_cfg->frequency < MHZ(1)) {
		clock_freq = SPIH_CLOCK_500K;
	} else if (spi_cfg->frequency < MHZ(2)) {
		clock_freq = SPIH_CLOCK_1M;
	} else if (spi_cfg->frequency < MHZ(4)) {
		clock_freq = SPIH_CLOCK_2M;
	} else if (spi_cfg->frequency < MHZ(8)) {
		clock_freq = SPIH_CLOCK_4M;
	} else if (spi_cfg->frequency < MHZ(16)) {
		clock_freq = SPIH_CLOCK_8M;
	} else {
		clock_freq = SPIH_CLOCK_16M;
	}
	spih->SPIHCFG &= ~(SPIH_MODE_MASK | SPIH_CLOCK_MASK);
	spih->SPIHCFG |= (mode << SPIH_MODE_POS) | (clock_freq << SPIH_CLOCK_POS);

	/* Get the frame length */
	frame_size = SPI_WORD_SIZE_GET(spi_cfg->operation);
	if (frame_size == 8) {
		data->bytes_per_frame = 1;
		spih->SPIHCTR &= ~SPIH_BUFF_16BITS;
	} else if (frame_size == 16) {
		data->bytes_per_frame = 2;
		spih->SPIHCTR |= SPIH_BUFF_16BITS;
	} else {
		LOG_ERR("Word sizes other than 8 and 16 bits are not supported");
		return -ENOTSUP;
	}

	/* Keep the context cfg info */
	ctx->config = spi_cfg;

	return 0;
}

static int spi_kb1200_transceive(const struct device *dev, const struct spi_config *spi_cfg,
				 const struct spi_buf_set *tx_bufs,
				 const struct spi_buf_set *rx_bufs)
{
	const struct kb1200_spi_config *config = dev->config;
	struct spih_regs *spih = config->base_addr;
	struct kb1200_spi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int ret;

	if (!spi_cfg) {
		LOG_ERR("spi_cfg error");
		return -EINVAL;
	}

	/* Lock the api context */
	spi_context_lock(ctx, NULL, NULL, NULL, spi_cfg);

	/* Setting new configuration */
	ret = spi_kb1200_configure(dev, spi_cfg);
	if (ret) {
		spi_context_release(ctx, 0);
		return ret;
	}

	/* Setup the context info */
	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, 1);
	if (!(spi_context_tx_on(ctx) || spi_context_rx_on(ctx))) {
		spi_context_release(ctx, 0);
		return 0;
	}

	/* CS Active */
	spih->SPIHCTR |= SPIH_CS_LOW;

	while (spi_context_tx_on(ctx) || spi_context_rx_on(ctx)) {
		uint16_t tx_frame = 0;
		uint16_t rx_frame;

		/* Get the tx_frame from tx_buf only when tx_buf != NULL */
		if (spi_context_tx_buf_on(ctx)) {
			if (data->bytes_per_frame == 1) {
				tx_frame = UNALIGNED_GET((uint8_t *)(ctx->tx_buf));
			} else {
				tx_frame = UNALIGNED_GET((uint16_t *)(ctx->tx_buf));
				tx_frame = BSWAP_16(tx_frame);
			}
		}
		/*
		 * The update is ignored if TX is off (tx_len == 0).
		 * Note: if tx_buf == NULL && tx_len != 0, the update still counts.
		 */
		spi_context_update_tx(ctx, data->bytes_per_frame, 1);
		spih->SPIHTBUF = tx_frame;

		/* wait busy flag */
		if (WAIT_FOR(!(spih->SPIHCTR & SPIH_BUSY_FLAG), 1000, NULL) == false) {
			LOG_ERR("Check Status BSY Timeout");
			break;
		}

		rx_frame = spih->SPIHRBUF;
		if (spi_context_rx_buf_on(ctx)) {
			if (data->bytes_per_frame == 1) {
				UNALIGNED_PUT(rx_frame, (uint8_t *)ctx->rx_buf);
			} else {
				rx_frame = BSWAP_16(rx_frame);
				UNALIGNED_PUT(rx_frame, (uint16_t *)ctx->rx_buf);
			}
		}

		spi_context_update_rx(ctx, data->bytes_per_frame, 1);
		/* Debug info */
		if (data->bytes_per_frame == 1) {
			LOG_DBG(" w: %02x, r: %02x", (uint8_t)tx_frame, (uint8_t)rx_frame);
		} else {
			LOG_DBG(" w: %04x, r: %04x", (uint16_t)tx_frame, (uint16_t)rx_frame);
		}
	}

	/* CS In-Active */
	if (!(spi_cfg->operation & SPI_HOLD_ON_CS)) {
		spih->SPIHCTR &= ~SPIH_CS_LOW;
	}
	spi_context_complete(ctx, dev, 0);
	spi_context_release(ctx, 0);

	return ret;
}

int spi_kb1200_release(const struct device *dev, const struct spi_config *spi_cfg)
{
	const struct kb1200_spi_config *config = dev->config;
	struct spih_regs *spih = config->base_addr;
	struct kb1200_spi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	spi_context_unlock_unconditionally(ctx);
	/* CS In-Active */
	spih->SPIHCTR &= ~SPIH_CS_LOW;

	return 0;
}

static struct spi_driver_api spi_kb1200_driver_api = {
	.transceive = spi_kb1200_transceive,
	.release = spi_kb1200_release,
};

static int spi_kb1200_init(const struct device *dev)
{
	int ret;
	const struct kb1200_spi_config *config = dev->config;
	struct spih_regs *spih = config->base_addr;
	struct kb1200_spi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		return ret;
	}

	/* Make sure the context is unlocked */
	spi_context_unlock_unconditionally(ctx);

	/*push pull and enable*/
	spih->SPIHCFG |= SPIH_PUSH_PULL | SPIH_FUNCTION_ENABLE;
	return 0;
}

#define KB1200_SPI_INIT(inst)                                                                      \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	static const struct kb1200_spi_config spi_kb1200_config_##inst = {                         \
		.base_addr = (struct spih_regs *)DT_INST_REG_ADDR(inst),                           \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
	};                                                                                         \
	static struct kb1200_spi_data kb1200_spi_data_##inst = {                                   \
		SPI_CONTEXT_INIT_LOCK(kb1200_spi_data_##inst, ctx),                                \
		SPI_CONTEXT_INIT_SYNC(kb1200_spi_data_##inst, ctx),                                \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(inst), ctx)};                          \
	DEVICE_DT_INST_DEFINE(inst, &spi_kb1200_init, NULL, &kb1200_spi_data_##inst,               \
			      &spi_kb1200_config_##inst, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,    \
			      &spi_kb1200_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KB1200_SPI_INIT)
