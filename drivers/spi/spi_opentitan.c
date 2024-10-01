/*
 * Copyright (c) 2023 Rivos Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_opentitan);

#include "spi_context.h"

#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi/rtio.h>
#include <soc.h>
#include <stdbool.h>

/* Register offsets within the SPI host register space. */
#define SPI_HOST_INTR_STATE_REG_OFFSET 0x00
#define SPI_HOST_INTR_ENABLE_REG_OFFSET 0x04
#define SPI_HOST_INTR_TEST_REG_OFFSET 0x08
#define SPI_HOST_ALERT_TEST_REG_OFFSET 0x0c
#define SPI_HOST_CONTROL_REG_OFFSET 0x10
#define SPI_HOST_STATUS_REG_OFFSET 0x14
#define SPI_HOST_CONFIGOPTS_REG_OFFSET 0x18
#define SPI_HOST_CSID_REG_OFFSET 0x1c
#define SPI_HOST_COMMAND_REG_OFFSET 0x20
#define SPI_HOST_RXDATA_REG_OFFSET 0x24
#define SPI_HOST_TXDATA_REG_OFFSET 0x28
#define SPI_HOST_ERROR_ENABLE_REG_OFFSET 0x2c
#define SPI_HOST_ERROR_STATUS_REG_OFFSET 0x30
#define SPI_HOST_EVENT_ENABLE_REG_OFFSET 0x34

/* Control register fields. */
#define SPI_HOST_CONTROL_OUTPUT_EN_BIT BIT(29)
#define SPI_HOST_CONTROL_SW_RST_BIT BIT(30)
#define SPI_HOST_CONTROL_SPIEN_BIT BIT(31)

/* Status register fields. */
#define SPI_HOST_STATUS_TXQD_MASK GENMASK(7, 0)
#define SPI_HOST_STATUS_RXQD_MASK GENMASK(15, 8)
#define SPI_HOST_STATUS_BYTEORDER_BIT BIT(22)
#define SPI_HOST_STATUS_RXEMPTY_BIT BIT(24)
#define SPI_HOST_STATUS_ACTIVE_BIT BIT(30)
#define SPI_HOST_STATUS_READY_BIT BIT(31)

/* Command register fields. */
#define SPI_HOST_COMMAND_LEN_MASK GENMASK(8, 0)
/* "Chip select active after transaction" */
#define SPI_HOST_COMMAND_CSAAT_BIT BIT(9)
#define SPI_HOST_COMMAND_SPEED_MASK GENMASK(11, 10)
#define SPI_HOST_COMMAND_SPEED_STANDARD (0 << 10)
#define SPI_HOST_COMMAND_SPEED_DUAL (1 << 10)
#define SPI_HOST_COMMAND_SPEED_QUAD (2 << 10)
#define SPI_HOST_COMMAND_DIRECTION_MASK GENMASK(13, 12)
#define SPI_HOST_COMMAND_DIRECTION_RX (0x1 << 12)
#define SPI_HOST_COMMAND_DIRECTION_TX (0x2 << 12)
#define SPI_HOST_COMMAND_DIRECTION_BOTH (0x3 << 12)

/* Configopts register fields. */
#define SPI_HOST_CONFIGOPTS_CPHA0_BIT BIT(30)
#define SPI_HOST_CONFIGOPTS_CPOL0_BIT BIT(31)

#define DT_DRV_COMPAT lowrisc_opentitan_spi

struct spi_opentitan_data {
	struct spi_context ctx;
};

struct spi_opentitan_cfg {
	uint32_t base;
	uint32_t f_input;
};

static int spi_config(const struct device *dev, uint32_t frequency,
		uint16_t operation)
{
	const struct spi_opentitan_cfg *cfg = dev->config;

	uint32_t reg;

	if (operation & SPI_HALF_DUPLEX) {
		return -ENOTSUP;
	}

	if (SPI_OP_MODE_GET(operation) != SPI_OP_MODE_MASTER) {
		return -ENOTSUP;
	}

	if (operation & SPI_MODE_LOOP) {
		return -ENOTSUP;
	}

	if (SPI_WORD_SIZE_GET(operation) != 8) {
		return -ENOTSUP;
	}

	if (IS_ENABLED(CONFIG_SPI_EXTENDED_MODES) &&
		(operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
		return -ENOTSUP;
	}

	/* Most significant bit always transferred first. */
	if (operation & SPI_TRANSFER_LSB) {
		return -ENOTSUP;
	}

	/* Set the SPI frequency, polarity, and clock phase in CONFIGOPTS register.
	 * Applied divider (divides f_in / 2) is CLKDIV register (16 bit) + 1.
	 */
	reg = cfg->f_input / 2 / frequency;
	if (reg > 0xffffu) {
		reg = 0xffffu;
	} else if (reg > 0) {
		reg--;
	}
	/* Setup phase */
	if (operation & SPI_MODE_CPHA) {
		reg |= SPI_HOST_CONFIGOPTS_CPHA0_BIT;
	}
	/* Setup polarity. */
	if (operation & SPI_MODE_CPOL) {
		reg |= SPI_HOST_CONFIGOPTS_CPOL0_BIT;
	}
	sys_write32(reg, cfg->base + SPI_HOST_CONFIGOPTS_REG_OFFSET);

	return 0;
}

static bool spi_opentitan_rx_available(const struct spi_opentitan_cfg *cfg)
{
	/* Rx bytes are available if Tx FIFO is non-empty. */
	return !(sys_read32(cfg->base + SPI_HOST_STATUS_REG_OFFSET) & SPI_HOST_STATUS_RXEMPTY_BIT);
}

static void spi_opentitan_xfer(const struct device *dev, const bool gpio_cs_control)
{
	const struct spi_opentitan_cfg *cfg = dev->config;
	struct spi_opentitan_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	while (spi_context_tx_on(ctx) || spi_context_rx_on(ctx)) {
		const size_t segment_len = MAX(ctx->tx_len, ctx->rx_len);
		uint32_t host_command_reg;

		/* Setup transaction duplex. */
		if (!spi_context_tx_on(ctx)) {
			host_command_reg = SPI_HOST_COMMAND_DIRECTION_RX;
		} else if (!spi_context_rx_on(ctx)) {
			host_command_reg = SPI_HOST_COMMAND_DIRECTION_TX;
		} else {
			host_command_reg = SPI_HOST_COMMAND_DIRECTION_BOTH;
		}

		size_t tx_bytes_to_queue = spi_context_tx_buf_on(ctx) ? ctx->tx_len : 0;

		/* First place Tx bytes in FIFO, packed four to a word. */
		while (tx_bytes_to_queue > 0) {
			uint32_t fifo_word = 0;

			for (int byte = 0; byte < 4; ++byte) {
				if (tx_bytes_to_queue == 0) {
					break;
				}
				fifo_word |= *ctx->tx_buf << (8 * byte);
				spi_context_update_tx(ctx, 1, 1);
				tx_bytes_to_queue--;
			}
			sys_write32(fifo_word, cfg->base + SPI_HOST_TXDATA_REG_OFFSET);
		}

		/* Keep CS asserted if another Tx segment remains or if two more Rx
		 * segments remain (because we will handle one Rx segment after the
		 * forthcoming transaction).
		 */
		if (ctx->tx_count > 0 || ctx->rx_count > 1)  {
			host_command_reg |= SPI_HOST_COMMAND_CSAAT_BIT;
		}
		/* Segment length field holds COMMAND.LEN + 1. */
		host_command_reg |= segment_len - 1;

		/* Issue transaction. */
		sys_write32(host_command_reg, cfg->base + SPI_HOST_COMMAND_REG_OFFSET);

		size_t rx_bytes_to_read = spi_context_rx_buf_on(ctx) ? ctx->rx_len : 0;

		/* Read from Rx FIFO as required. */
		while (rx_bytes_to_read > 0) {
			while (!spi_opentitan_rx_available(cfg)) {
				;
			}
			uint32_t rx_word = sys_read32(cfg->base +
				SPI_HOST_RXDATA_REG_OFFSET);
			for (int byte = 0; byte < 4; ++byte) {
				if (rx_bytes_to_read == 0) {
					break;
				}
				*ctx->rx_buf = (rx_word >> (8 * byte)) & 0xff;
				spi_context_update_rx(ctx, 1, 1);
				rx_bytes_to_read--;
			}
		}
	}

	/* Deassert the CS line if required. */
	if (gpio_cs_control) {
		spi_context_cs_control(ctx, false);
	}

	spi_context_complete(ctx, dev, 0);
}

static int spi_opentitan_init(const struct device *dev)
{
	const struct spi_opentitan_cfg *cfg = dev->config;
	struct spi_opentitan_data *data = dev->data;
	int err;

	/* Place SPI host peripheral in reset and wait for reset to complete. */
	sys_write32(SPI_HOST_CONTROL_SW_RST_BIT,
		cfg->base + SPI_HOST_CONTROL_REG_OFFSET);
	while (sys_read32(cfg->base + SPI_HOST_STATUS_REG_OFFSET)
			& (SPI_HOST_STATUS_ACTIVE_BIT | SPI_HOST_STATUS_TXQD_MASK |
				SPI_HOST_STATUS_RXQD_MASK)) {
		;
	}
	/* Clear reset and enable SPI host peripheral. */
	sys_write32(SPI_HOST_CONTROL_OUTPUT_EN_BIT | SPI_HOST_CONTROL_SPIEN_BIT,
		cfg->base + SPI_HOST_CONTROL_REG_OFFSET);

	err = spi_context_cs_configure_all(&data->ctx);
	if (err < 0) {
		return err;
	}

	/* Make sure the context is unlocked */
	spi_context_unlock_unconditionally(&data->ctx);
	return 0;
}

static int spi_opentitan_transceive(const struct device *dev,
			  const struct spi_config *config,
			  const struct spi_buf_set *tx_bufs,
			  const struct spi_buf_set *rx_bufs)
{
	int rc = 0;
	bool gpio_cs_control = false;
	struct spi_opentitan_data *data = dev->data;

	/* Lock the SPI Context */
	spi_context_lock(&data->ctx, false, NULL, NULL, config);

	/* Configure the SPI bus */
	data->ctx.config = config;
	rc = spi_config(dev, config->frequency, config->operation);
	if (rc < 0) {
		spi_context_release(&data->ctx, rc);
		return rc;
	}

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

	/* Assert the CS line. HW will always assert the CS pin identified by CSID
	 * (default CSID: 0), so GPIO CS control will work in addition to HW
	 * asserted (and presumably ignored) CS.
	 */
	if (config->cs) {
		gpio_cs_control = true;
		spi_context_cs_control(&data->ctx, true);
	}

	/* Perform transfer */
	spi_opentitan_xfer(dev, gpio_cs_control);

	rc = spi_context_wait_for_completion(&data->ctx);

	spi_context_release(&data->ctx, rc);

	return rc;
}

#ifdef CONFIG_SPI_ASYNC
static int spi_opentitan_transceive_async(const struct device *dev,
					const struct spi_config *spi_cfg,
					const struct spi_buf_set *tx_bufs,
					const struct spi_buf_set *rx_bufs,
					spi_callback_t cb,
					void *userdata)
{
	return -ENOTSUP;
}
#endif

static int spi_opentitan_release(const struct device *dev,
			   const struct spi_config *config)
{
	struct spi_opentitan_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);
	return 0;
}

/* Device Instantiation */

static const struct spi_driver_api spi_opentitan_api = {
	.transceive = spi_opentitan_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_opentitan_transceive_async,
#endif
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
	.release = spi_opentitan_release,
};

#define SPI_INIT(n) \
	static struct spi_opentitan_data spi_opentitan_data_##n = { \
		SPI_CONTEXT_INIT_LOCK(spi_opentitan_data_##n, ctx), \
		SPI_CONTEXT_INIT_SYNC(spi_opentitan_data_##n, ctx), \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)	\
	}; \
	static struct spi_opentitan_cfg spi_opentitan_cfg_##n = { \
		.base = DT_INST_REG_ADDR(n), \
		.f_input = DT_INST_PROP(n, clock_frequency), \
	}; \
	DEVICE_DT_INST_DEFINE(n, \
			spi_opentitan_init, \
			NULL, \
			&spi_opentitan_data_##n, \
			&spi_opentitan_cfg_##n, \
			POST_KERNEL, \
			CONFIG_SPI_INIT_PRIORITY, \
			&spi_opentitan_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_INIT)
