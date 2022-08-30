/*
 * Copyright (c) 2019 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT litex_spi

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_litespi);
#include "spi_litespi.h"
#include <stdbool.h>

/* Helper Functions */
static int spi_config(const struct spi_config *config, uint16_t *control)
{
	uint8_t cs = 0x00;

	if (config->slave != 0) {
		if (config->slave >= SPI_MAX_CS_SIZE) {
			LOG_ERR("More slaves than supported");
			return -ENOTSUP;
		}
		cs = (uint8_t)(config->slave);
	}

	if (config->operation & SPI_HALF_DUPLEX) {
		LOG_ERR("Half-duplex not supported");
		return -ENOTSUP;
	}

	if (SPI_WORD_SIZE_GET(config->operation) != 8) {
		LOG_ERR("Word size must be %d", SPI_WORD_SIZE);
		return -ENOTSUP;
	}

	if (config->operation & SPI_CS_ACTIVE_HIGH) {
		LOG_ERR("CS active high not supported");
		return -ENOTSUP;
	}

	if (config->operation & SPI_LOCK_ON) {
		LOG_ERR("Lock On not supported");
		return -ENOTSUP;
	}

	if (IS_ENABLED(CONFIG_SPI_EXTENDED_MODES) &&
	    (config->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
		LOG_ERR("Only supports single mode");
		return -ENOTSUP;
	}

	if (config->operation & SPI_TRANSFER_LSB) {
		LOG_ERR("LSB first not supported");
		return -ENOTSUP;
	}

	if (config->operation & (SPI_MODE_CPOL | SPI_MODE_CPHA)) {
		LOG_ERR("Only supports CPOL=CPHA=0");
		return -ENOTSUP;
	}

	if (config->operation & SPI_OP_MODE_SLAVE) {
		LOG_ERR("Slave mode not supported");
		return -ENOTSUP;
	}

	/* Set Loopback */
	if (config->operation & SPI_MODE_LOOP) {
		litex_write8(SPI_ENABLE, SPI_LOOPBACK_ADDR);
	}
	/* Set word size */
	*control = (uint16_t) (SPI_WORD_SIZE_GET(config->operation)
			<< POSITION_WORD_SIZE);
	/* Write configurations */
	litex_write8(cs, SPI_CS_ADDR);
	litex_write16(*control, SPI_CONTROL_ADDR);

	return 0;
}

static void spi_litespi_send(const struct device *dev, uint8_t frame,
		             uint16_t control)
{
	/* Write frame to register */
	litex_write8(frame, SPI_MOSI_DATA_ADDR);
	/* Start the transfer */
	litex_write16(control | SPI_ENABLE, SPI_CONTROL_ADDR);
	/* Wait until the transfer ends */
	while (!(litex_read8(SPI_STATUS_ADDR)))
		;
}

static uint8_t spi_litespi_recv(void)
{
    /* Return data inside MISO register */
	return litex_read8(SPI_MISO_DATA_ADDR);
}

static void spi_litespi_xfer(const struct device *dev,
			     const struct spi_config *config,
			     uint16_t control)
{
	struct spi_context *ctx = &SPI_DATA(dev)->ctx;
	uint32_t send_len = spi_context_longest_current_buf(ctx);
	uint8_t read_data;

	for (uint32_t i = 0; i < send_len; i++) {
		/* Send a frame */
		if (i < ctx->tx_len) {
			spi_litespi_send(dev, (uint8_t) (ctx->tx_buf)[i],
					control);
		} else {
			/* Send dummy bytes */
			spi_litespi_send(dev, 0, control);
		}
		/* Receive a frame */
		read_data = spi_litespi_recv();
		if (i < ctx->rx_len) {
			ctx->rx_buf[i] = read_data;
		}
	}
	spi_context_complete(ctx, 0);
}

/* API Functions */

static int spi_litespi_init(const struct device *dev)
{
	return 0;
}

static int spi_litespi_transceive(const struct device *dev,
				  const struct spi_config *config,
				  const struct spi_buf_set *tx_bufs,
				  const struct spi_buf_set *rx_bufs)
{
	uint16_t control = 0;

	spi_config(config, &control);
	spi_context_buffers_setup(&SPI_DATA(dev)->ctx, tx_bufs, rx_bufs, 1);
	spi_litespi_xfer(dev, config, control);
	return 0;
}

#ifdef CONFIG_SPI_ASYNC
static int spi_litespi_transceive_async(const struct device *dev,
					const struct spi_config *config,
					const struct spi_buf_set *tx_bufs,
					const struct spi_buf_set *rx_bufs,
					struct k_poll_signal *async)
{
	return -ENOTSUP;
}
#endif /* CONFIG_SPI_ASYNC */

static int spi_litespi_release(const struct device *dev,
			       const struct spi_config *config)
{
	if (!(litex_read8(SPI_STATUS_ADDR))) {
		return -EBUSY;
	}
	return 0;
}

/* Device Instantiation */
static struct spi_driver_api spi_litespi_api = {
	.transceive = spi_litespi_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_litespi_transceive_async,
#endif /* CONFIG_SPI_ASYNC */
	.release = spi_litespi_release,
};

#define SPI_INIT(n)	\
	static struct spi_litespi_data spi_litespi_data_##n = { \
		SPI_CONTEXT_INIT_LOCK(spi_litespi_data_##n, ctx), \
		SPI_CONTEXT_INIT_SYNC(spi_litespi_data_##n, ctx), \
	}; \
	static struct spi_litespi_cfg spi_litespi_cfg_##n = { \
		.base = DT_INST_REG_ADDR_BY_NAME(n, control), \
	}; \
	DEVICE_DT_INST_DEFINE(n, \
			spi_litespi_init, \
			NULL, \
			&spi_litespi_data_##n, \
			&spi_litespi_cfg_##n, \
			POST_KERNEL, \
			CONFIG_SPI_INIT_PRIORITY, \
			&spi_litespi_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_INIT)
