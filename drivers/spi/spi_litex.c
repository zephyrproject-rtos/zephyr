/*
 * Copyright (c) 2019 Antmicro <www.antmicro.com>
 * Copyright (c) 2024 Vogl Electronic GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT litex_spi

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_litex);

#include "spi_litex_common.h"

#define POSITION_WORD_SIZE 8

struct spi_litex_data {
	struct spi_context ctx;
	uint8_t dfs;	/* dfs in bytes: 1,2,3 or 4 */
};

struct spi_litex_cfg {
	uint32_t control_addr;
	uint32_t status_addr;
	uint32_t mosi_addr;
	uint32_t miso_addr;
	uint32_t cs_addr;
	uint32_t loopback_addr;
	uint32_t clk_divider_addr;
	bool clk_divider_exists;
	int data_width;
	int max_cs;
};

static void spi_set_frequency(const struct device *dev, const struct spi_config *config)
{
	const struct spi_litex_cfg *dev_config = dev->config;

	if (!dev_config->clk_divider_exists) {
		/* The clk_divider is optional, thats why we check. */
		LOG_WRN("No clk_divider found, can't change frequency");
		return;
	}

	uint16_t divisor = DIV_ROUND_UP(sys_clock_hw_cycles_per_sec(), config->frequency);

	litex_write16(divisor, dev_config->clk_divider_addr);
}

/* Helper Functions */
static int spi_config(const struct device *dev, const struct spi_config *config, uint16_t *control)
{
	const struct spi_litex_cfg *dev_config = dev->config;
	struct spi_litex_data *dev_data = dev->data;

	if (config->slave >= dev_config->max_cs) {
		LOG_ERR("More slaves than supported");
		return -ENOTSUP;
	}

	if (config->operation & SPI_HALF_DUPLEX) {
		LOG_ERR("Half-duplex not supported");
		return -ENOTSUP;
	}

	if (SPI_WORD_SIZE_GET(config->operation) > dev_config->data_width) {
		LOG_ERR("Word size must be <= %d", dev_config->data_width);
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
	if (!litex_read8(dev_config->loopback_addr) != !(config->operation & SPI_MODE_LOOP)) {
		litex_write8(((config->operation & SPI_MODE_LOOP) ? 0x1 : 0x0),
			     dev_config->loopback_addr);
	}
	/* Set word size */
	*control = (uint16_t) (SPI_WORD_SIZE_GET(config->operation)
			<< POSITION_WORD_SIZE);

	dev_data->dfs = get_dfs_value(config);

	/* Write configurations */
	litex_write16(*control, dev_config->control_addr);

	spi_set_frequency(dev, config);

	return 0;
}

static void spi_litex_send(const struct device *dev, uint32_t frame,
			   uint16_t control)
{
	const struct spi_litex_cfg *dev_config = dev->config;
	/* Write frame to register */
	litex_write32(frame, dev_config->mosi_addr);
	/* Start the transfer */
	litex_write16(control | BIT(0), dev_config->control_addr);
	/* Wait until the transfer ends */
	while (!(litex_read8(dev_config->status_addr) & BIT(0))) {
		;/* Wait */
	}
}

static uint32_t spi_litex_recv(const struct device *dev)
{
	const struct spi_litex_cfg *dev_config = dev->config;

	/* Return data inside MISO register */
	return litex_read32(dev_config->miso_addr);
}

static void spi_litex_xfer(const struct device *dev,
			   const struct spi_config *config,
			   uint16_t control)
{
	const struct spi_litex_cfg *dev_config = dev->config;
	struct spi_litex_data *dev_data = dev->data;
	struct spi_context *ctx = &dev_data->ctx;
	uint32_t txd, rxd;

	/* Set CS */
	litex_write16(BIT(config->slave), dev_config->cs_addr);

	do {
		/* Send a frame */
		if (spi_context_tx_buf_on(ctx)) {
			litex_spi_tx_put(dev_data->dfs, &txd, ctx->tx_buf);
		} else {
			txd = 0U;
		}

		LOG_DBG("txd: 0x%x", txd);
		spi_litex_send(dev, txd, control);

		spi_context_update_tx(ctx, dev_data->dfs, 1);

		rxd = spi_litex_recv(dev);
		LOG_DBG("rxd: 0x%x", rxd);

		if (spi_context_rx_buf_on(ctx)) {
			litex_spi_rx_put(dev_data->dfs, &rxd, ctx->rx_buf);
		}

		spi_context_update_rx(ctx, dev_data->dfs, 1);

	} while (spi_context_tx_on(ctx) || spi_context_rx_on(ctx));

	spi_context_complete(ctx, dev, 0);

	/* Clear CS */
	litex_write16(0, dev_config->cs_addr);
}

/* API Functions */

static int spi_litex_transceive(const struct device *dev, const struct spi_config *config,
				const struct spi_buf_set *tx_bufs,
				const struct spi_buf_set *rx_bufs)
{
	struct spi_litex_data *dev_data = dev->data;
	uint16_t control = 0;
	int ret = 0;

	ret = spi_config(dev, config, &control);
	if (ret < 0) {
		return ret;
	}
	spi_context_buffers_setup(&dev_data->ctx, tx_bufs, rx_bufs, dev_data->dfs);
	spi_litex_xfer(dev, config, control);
	return 0;
}

#ifdef CONFIG_SPI_ASYNC
static int spi_litex_transceive_async(const struct device *dev, const struct spi_config *config,
				      const struct spi_buf_set *tx_bufs,
				      const struct spi_buf_set *rx_bufs,
				      struct k_poll_signal *async)
{
	return -ENOTSUP;
}
#endif /* CONFIG_SPI_ASYNC */

static int spi_litex_release(const struct device *dev, const struct spi_config *config)
{
	const struct spi_litex_cfg *dev_config = dev->config;

	if (!(litex_read8(dev_config->status_addr) & BIT(0))) {
		return -EBUSY;
	}
	return 0;
}

/* Device Instantiation */
static const struct spi_driver_api spi_litex_api = {
	.transceive = spi_litex_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_litex_transceive_async,
#endif /* CONFIG_SPI_ASYNC */
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
	.release = spi_litex_release,
};

#define SPI_INIT(n)                                                                                \
	static struct spi_litex_data spi_litex_data_##n = {                                        \
		SPI_CONTEXT_INIT_LOCK(spi_litex_data_##n, ctx),                                    \
		SPI_CONTEXT_INIT_SYNC(spi_litex_data_##n, ctx),                                    \
	};                                                                                         \
	static struct spi_litex_cfg spi_litex_cfg_##n = {                                          \
		.control_addr = DT_INST_REG_ADDR_BY_NAME(n, control),                              \
		.status_addr = DT_INST_REG_ADDR_BY_NAME(n, status),                                \
		.mosi_addr = DT_INST_REG_ADDR_BY_NAME(n, mosi),                                    \
		.miso_addr = DT_INST_REG_ADDR_BY_NAME(n, miso),                                    \
		.cs_addr = DT_INST_REG_ADDR_BY_NAME(n, cs),                                        \
		.loopback_addr = DT_INST_REG_ADDR_BY_NAME(n, loopback),                            \
		.clk_divider_exists = DT_INST_REG_HAS_NAME(n, clk_divider),                        \
		.clk_divider_addr = DT_INST_REG_ADDR_BY_NAME_OR(n, clk_divider, 0),                \
		.data_width = DT_INST_PROP(n, data_width),                                         \
		.max_cs = DT_INST_PROP(n, max_cs),                                                 \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n,                                                                   \
			NULL,                                                                      \
			NULL,                                                                      \
			&spi_litex_data_##n,                                                       \
			&spi_litex_cfg_##n,                                                        \
			POST_KERNEL,                                                               \
			CONFIG_SPI_INIT_PRIORITY,                                                  \
			&spi_litex_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_INIT)
