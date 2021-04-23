/*
 * Copyright (c) 2021 Open Cosmos ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT atmel_sam0_spi_quadspi

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(qspi_sam0);

#include "spi_context.h"
#include <errno.h>
#include <device.h>
#include <drivers/spi.h>
#include <soc.h>
#include <drivers/dma.h>

struct sam0_quadspi_config {
	Qspi *regs;
	volatile uint32_t *mclk_ahb;
	volatile uint32_t *mclk_apb;
	uint32_t mclk_mask_ahb;
	uint32_t mclk_mask_apb;
	uint32_t mclk_mask_ahb_x2;
};

struct sam0_quadspi_data {
	struct spi_context ctx;
};

static int sam0_quadspi_configure(const struct device *dev,
				  const struct spi_config *config)
{
	const struct sam0_quadspi_config *cfg = dev->config;
	struct sam0_quadspi_data *data = dev->data;
	Qspi *regs = cfg->regs;
	QSPI_CTRLA_Type ctrla = { .reg = 0 };
	QSPI_CTRLB_Type ctrlb = { .reg = 0 };
	QSPI_BAUD_Type ctrl_baud = { .reg = 0 };
	int divider;

	if (spi_context_configured(&data->ctx, config)) {
		return 0;
	}

	if (SPI_OP_MODE_GET(config->operation) != SPI_OP_MODE_MASTER) {
		LOG_ERR("Slave mode is not supported");
		return -ENOTSUP;
	}

	if ((config->operation & SPI_MODE_LOOP) != 0U) {
		LOG_ERR("Loopback mode is not supported");
		return -ENOTSUP;
	}

	if ((config->operation & SPI_TRANSFER_LSB) != 0U) {
		LOG_ERR("LSB first mode is not supported");
		return -ENOTSUP;
	}

	if ((config->operation & SPI_MODE_CPOL) != 0U) {
		ctrl_baud.bit.CPOL = 1;
	}

	if ((config->operation & SPI_MODE_CPHA) != 0U) {
		ctrl_baud.bit.CPHA = 1;
	}

	/* only release the CS when LASTXFER is asserted. */
	ctrlb.bit.CSMODE = QSPI_CTRLB_CSMODE_LASTXFER_Val;

	ctrla.bit.ENABLE = 1;

	if (SPI_WORD_SIZE_GET(config->operation) != 8) {
		return -ENOTSUP;
	}

	/* 8 bits per transfer */
	ctrlb.bit.DATALEN = 0;

	/* Use the requested or next highest possible frequency */
	divider = (SOC_ATMEL_SAM0_MCK_FREQ_HZ / config->frequency) -1;
	divider = MAX(0, MIN(UINT8_MAX, divider));
	ctrl_baud.bit.BAUD = divider;

	/* Update the configuration */
	regs->CTRLA.bit.ENABLE = 0;
	regs->CTRLB = ctrlb;
	regs->BAUD = ctrl_baud;
	regs->CTRLA = ctrla;

	data->ctx.config = config;

	return 0;
}

static int sam0_quadspi_init(const struct device *dev)
{
	const struct sam0_quadspi_config *cfg = dev->config;
	struct sam0_quadspi_data *data = dev->data;
	Qspi *regs = cfg->regs;

	/* Enable the MCLK */
	*cfg->mclk_ahb |= (cfg->mclk_mask_ahb | cfg->mclk_mask_ahb_x2);
	*cfg->mclk_apb |= cfg->mclk_mask_apb;

	/* Disable all SPI interrupts */
	regs->INTENCLR.reg = QSPI_INTENCLR_MASK;

	spi_context_unlock_unconditionally(&data->ctx);

	/* The device will be configured and enabled when transceive
	 * is called.
	 */

	return 0;
}

static void sam0_quadspi_send(const struct device *dev, uint8_t frame)
{
	const struct sam0_quadspi_config *cfg = dev->config;
	Qspi *regs = cfg->regs;

	while (regs->INTFLAG.bit.DRE == 0) {
	}

	regs->TXDATA.bit.DATA = frame;
}

static uint8_t sam0_quadspi_recv(const struct device *dev)
{
	const struct sam0_quadspi_config *cfg = dev->config;
	Qspi *regs = cfg->regs;

	while (regs->INTFLAG.bit.RXC == 0) {
	}

	return (uint8_t) regs->RXDATA.bit.DATA;
}

static int sam0_quadspi_transceive_sync(const struct device *dev,
	const struct spi_config *config,
	const struct spi_buf_set *tx_bufs,
	const struct spi_buf_set *rx_bufs)
{
	const struct sam0_quadspi_config *cfg = dev->config;
	Qspi *regs = cfg->regs;
	struct sam0_quadspi_data *data = dev->data;
	int err;
	size_t cur_xfer_len;
	size_t i;
	uint8_t rx_byte;

	spi_context_lock(&data->ctx, false, NULL, config);

	err = sam0_quadspi_configure(dev, config);
	if (err != 0) {
		goto done;
	}

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

	while (spi_context_tx_buf_on(&data->ctx)
	||     spi_context_rx_buf_on(&data->ctx)) {
		cur_xfer_len = spi_context_longest_current_buf(&data->ctx);

		for (i = 0; i < cur_xfer_len; i++) {

			/* Write byte */
			if (spi_context_tx_buf_on(&data->ctx)) {
				sam0_quadspi_send(dev, *data->ctx.tx_buf);
				spi_context_update_tx(&data->ctx, 1, 1);
			} else {
				sam0_quadspi_send(dev, 0);
			}

			/* Get received byte */
			rx_byte = sam0_quadspi_recv(dev);

			/* Store received byte if rx buffer is on */
			if (spi_context_rx_on(&data->ctx)) {
				*data->ctx.rx_buf = rx_byte;
				spi_context_update_rx(&data->ctx, 1, 1);
			}
		}
	}

	/* Set lastXfer bit so that CS will be deasserted */
	regs->CTRLA.reg = (1 << QSPI_CTRLA_LASTXFER_Pos)
					| (1 << QSPI_CTRLA_ENABLE_Pos);

done:
	spi_context_release(&data->ctx, err);
	return err;
}

static int sam0_quadspi_release(const struct device *dev,
				const struct spi_config *config)
{
	struct sam0_quadspi_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static const struct spi_driver_api sam0_quadspi_driver_api = {
	.transceive = sam0_quadspi_transceive_sync,
	.release = sam0_quadspi_release,
};

#define MCLK_ADDR_OFFSET_DT_BY_NAME(n, name)					\
	(DT_REG_ADDR(DT_INST_PHANDLE_BY_NAME(n, clocks, name)) +	\
	 DT_INST_CLOCKS_CELL_BY_NAME(n, name, offset))

#define SAM0_QUADSPI_DEFINE_CONFIG(n)														\
	static const struct sam0_quadspi_config sam0_quadspi_config_##n = {						\
		.regs = (Qspi *)DT_INST_REG_ADDR(n),												\
		.mclk_ahb = (volatile uint32_t *)MCLK_ADDR_OFFSET_DT_BY_NAME(n, qspi_ahb_clock),	\
		.mclk_apb = (volatile uint32_t *)MCLK_ADDR_OFFSET_DT_BY_NAME(n, qspi_apb_clock),	\
		.mclk_mask_ahb = BIT(DT_INST_CLOCKS_CELL_BY_NAME(n, qspi_ahb_clock, bit)),			\
		.mclk_mask_apb = BIT(DT_INST_CLOCKS_CELL_BY_NAME(n, qspi_apb_clock, bit)),			\
		.mclk_mask_ahb_x2 = BIT(DT_INST_CLOCKS_CELL_BY_NAME(n, qspi_2x_ahb_clock, bit)),	\
	}

#define SAM0_QUADSPI_DEVICE_INIT(n)									\
	SAM0_QUADSPI_DEFINE_CONFIG(n);									\
	static struct sam0_quadspi_data sam0_quadspi_dev_data_##n = {	\
		SPI_CONTEXT_INIT_LOCK(sam0_quadspi_dev_data_##n, ctx),		\
		SPI_CONTEXT_INIT_LOCK(sam0_quadspi_dev_data_##n, ctx),		\
	};																\
	DEVICE_DT_INST_DEFINE(n,										\
		&sam0_quadspi_init,											\
		device_pm_control_nop,										\
		&sam0_quadspi_dev_data_##n,									\
		&sam0_quadspi_config_##n,									\
		POST_KERNEL,												\
		CONFIG_SPI_INIT_PRIORITY,									\
		&sam0_quadspi_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SAM0_QUADSPI_DEVICE_INIT)
