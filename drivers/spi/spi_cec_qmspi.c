/*
 * Copyright (c) 2017 Crypta Labs Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SPI_LEVEL
#include <logging/sys_log.h>

#include <errno.h>
#include <spi.h>
#include <soc.h>

#include "spi_context.h"

typedef void (*irq_config_func_t)(struct device *port);

struct spi_qmspi_data {
	struct spi_context ctx;
};

struct spi_qmspi_config {
	QMSPI_INST_Type *spi;
};

static inline struct spi_qmspi_data *get_dev_data(struct device *dev)
{
	return dev->driver_data;
}

static inline const struct spi_qmspi_config *get_dev_config(struct device *dev)
{
	return dev->config->config_info;
}

static int spi_qmspi_configure(struct device *dev,
			       const struct spi_config *spi_cfg)
{
	const struct spi_qmspi_config *cfg = get_dev_config(dev);
	struct spi_qmspi_data *data = get_dev_data(dev);
	QMSPI_INST_Type *spi = cfg->spi;

	if (SPI_OP_MODE_GET(spi_cfg->operation) == SPI_OP_MODE_SLAVE ||
	    (spi_cfg->operation & SPI_TRANSFER_LSB) ||
	    (spi_cfg->frequency == 0)) {
		return -ENOTSUP;
	}

	if (SPI_WORD_SIZE_GET(spi_cfg->operation) != 8) {
		return -ENOTSUP;
	}

	spi->QMSPI_MODE_b.CLOCK_DIVIDE =
		SYSCLK_DEFAULT_IOSC_HZ / spi_cfg->frequency;
	spi->QMSPI_MODE_b.ACTIVATE = 1;
	spi->QMSPI_MODE_b.CPOL =
		SPI_MODE_GET(spi_cfg->operation) ==  SPI_MODE_CPOL;
	spi->QMSPI_MODE_b.CHPA_MISO =
		SPI_MODE_GET(spi_cfg->operation) == SPI_MODE_CPHA;
	spi->QMSPI_MODE_b.CHPA_MOSI =
		SPI_MODE_GET(spi_cfg->operation) == SPI_MODE_CPHA;

	spi->QMSPI_CONTROL_b.CLOSE_TRANSFER_ENABLE =
		!(spi_cfg->operation & SPI_HOLD_ON_CS);
	spi->QMSPI_CONTROL_b.TRANSFER_UNITS = 1; /* Unit of byte */
	switch (spi_cfg->operation & SPI_LINES_MASK) {
	case SPI_LINES_SINGLE:
		spi->QMSPI_CONTROL_b.INTERFACE_MODE = 0;
		break;
#if 0
#error These modes require transfer direction which is not yet supported.
	case SPI_LINES_DUAL:
		spi->QMSPI_CONTROL_b.INTERFACE_MODE = 1;
		break;
	case SPI_LINES_QUAD:
		spi->QMSPI_CONTROL_b.INTERFACE_MODE = 2;
		break;
#endif
	default:
		return -ENOTSUP;
	}

	/* At this point, it's mandatory to set this on the context! */
	data->ctx.config = spi_cfg;

	spi_context_cs_configure(&data->ctx);

	SYS_LOG_DBG("Installed config %p: freq %uHz,"
		    " mode %u/%u/%u, slave %u, if_mode=%d",
		    spi_cfg, spi_cfg->frequency,
		    (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPOL) ? 1 : 0,
		    (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPHA) ? 1 : 0,
		    (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_LOOP) ? 1 : 0,
		    spi_cfg->slave,
		    spi->QMSPI_CONTROL_b.INTERFACE_MODE);

	return 0;
}

static void spi_qmspi_complete(struct spi_qmspi_data *data,
			       QMSPI_INST_Type *spi, int status)
{
#ifdef CONFIG_SPI_QMSPI_INTERRUPT
	spi->QMSPI_INTERRUPT_ENABLE_b.TRANSFER_COMPLETE_ENABLE = 0;
#endif

	spi_context_cs_control(&data->ctx, false);

	spi->QMSPI_EXECUTE_b.STOP = 1;
	spi->QMSPI_EXECUTE_b.CLEAR_DATA_BUFFER = 1;

	if (status) {
		spi->QMSPI_MODE_b.SOFT_RESET = 1;
		while (spi->QMSPI_MODE_b.SOFT_RESET)
			;
	}

#ifdef CONFIG_SPI_QMSPI_INTERRUPT
	spi_context_complete(&data->ctx, status);
#endif
}

static int spi_qmspi_shift(QMSPI_INST_Type *spi, struct spi_qmspi_data *data)
{
	u8_t byte;

	if (spi_context_tx_on(&data->ctx)) {
		byte = 0;
		if (data->ctx.tx_buf) {
			byte = UNALIGNED_GET((u8_t *)(data->ctx.tx_buf));
		}
		spi_context_update_tx(&data->ctx, 1, 1);
		while (!spi->QMSPI_STATUS_b.TRANSMIT_BUFFER_EMPTY)
			;
		sys_write8(byte, (mem_addr_t)&spi->QMSPI_TRAMSMIT_BUFFER);
	}

	if (spi_context_rx_on(&data->ctx)) {
		while (spi->QMSPI_STATUS_b.RECEIVE_BUFFER_EMPTY)
			;
		byte = sys_read8((mem_addr_t)&spi->QMSPI_RECEIVE_BUFFER);
		if (data->ctx.rx_buf) {
			UNALIGNED_PUT(byte, (u8_t *)data->ctx.rx_buf);
		}
		spi_context_update_rx(&data->ctx, 1, 1);
	}

	/* Check for error bits 0b11100 */
	return spi->QMSPI_STATUS & 0x1c;
}

static int transceive(struct device *dev,
		      const struct spi_config *spi_cfg,
		      const struct spi_buf_set *tx_bufs,
		      const struct spi_buf_set *rx_bufs,
		      struct k_poll_signal *async)
{
	const struct spi_qmspi_config *cfg = get_dev_config(dev);
	struct spi_qmspi_data *data = get_dev_data(dev);
	QMSPI_INST_Type *spi = cfg->spi;
	int ret;

#ifndef CONFIG_SPI_CEC_QMSPI_INTERRUPT
	if (async != NULL) {
		return -ENOTSUP;
	}
#endif

	spi_context_lock(&data->ctx, async != NULL, async);

	ret = spi_qmspi_configure(dev, spi_cfg);
	if (ret) {
		return ret;
	}

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

	spi->QMSPI_CONTROL_b.TRANSFER_LENGTH =
		spi_context_transfer_length(&data->ctx);

	spi->QMSPI_CONTROL_b.RX_TRANSFER_ENABLE = rx_bufs ? 1 : 0;
	spi->QMSPI_CONTROL_b.TX_TRANSFER_ENABLE = tx_bufs ? 1 : 0;

	spi_context_cs_control(&data->ctx, true);
	spi->QMSPI_EXECUTE_b.START = 1;

#ifdef CONFIG_SPI_CEC_QMSPI_INTERRUPT
#error Not supported yet.
	spi->QMSPI_INTERRUPT_ENABLE_b.TRANSFER_COMPLETE_ENABLE = 1;
	ret = spi_context_wait_for_completion(&data->ctx);
#else
	do {
		ret = spi_qmspi_shift(spi, data);
	} while (!ret &&
		 (spi_context_tx_on(&data->ctx) ||
		  spi_context_rx_on(&data->ctx)));

	while (!spi->QMSPI_STATUS_b.TRANSMIT_BUFFER_EMPTY)
		;

	spi_qmspi_complete(data, spi, ret);
#endif

	spi_context_release(&data->ctx, ret);

	if (ret) {
		SYS_LOG_ERR("error mask 0x%x", ret);
	}

	return ret ? -EIO : 0;
}

static int spi_qmspi_transceive(struct device *dev,
				const struct spi_config *spi_cfg,
				const struct spi_buf_set *tx_bufs,
				const struct spi_buf_set *rx_bufs)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, NULL);
}

#ifdef CONFIG_POLL
static int spi_qmspi_transceive_async(struct device *dev,
				      const struct spi_config *spi_cfg,
				      const struct spi_buf_set *tx_bufs,
				      const struct spi_buf_set *rx_bufs,
				      struct k_poll_signal *async)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, async);
}
#endif /* CONFIG_POLL */

static int spi_qmspi_release(struct device *dev,
			     const struct spi_config *config)
{
	const struct spi_qmspi_config *cfg = get_dev_config(dev);
	struct spi_qmspi_data *data = get_dev_data(dev);
	QMSPI_INST_Type *spi = cfg->spi;

	spi_context_unlock_unconditionally(&data->ctx);
	spi->QMSPI_MODE_b.ACTIVATE = 0;

	return 0;
}

static const struct spi_driver_api api_funcs = {
	.transceive = spi_qmspi_transceive,
#ifdef CONFIG_POLL
	.transceive_async = spi_qmspi_transceive_async,
#endif
	.release = spi_qmspi_release,
};

static int spi_qmspi_init(struct device *dev)
{
	const struct spi_qmspi_config *cfg = dev->config->config_info;
	struct spi_qmspi_data *data = dev->driver_data;
	QMSPI_INST_Type *spi = cfg->spi;

	spi_context_unlock_unconditionally(&data->ctx);

	/* Reset block */
	spi->QMSPI_MODE_b.ACTIVATE = 1;
	spi->QMSPI_MODE_b.SOFT_RESET = 1;
	while (spi->QMSPI_MODE_b.SOFT_RESET)
		;
	spi->QMSPI_MODE_b.ACTIVATE = 0;

	return 0;
}

#ifdef CONFIG_SPI_0

static const struct spi_qmspi_config spi_qmspi_cfg_0 = {
	.spi = (QMSPI_INST_Type *) QMSPI_INST,
};

static struct spi_qmspi_data spi_qmspi_dev_data_0 = {
	SPI_CONTEXT_INIT_LOCK(spi_qmspi_dev_data_0, ctx),
	SPI_CONTEXT_INIT_SYNC(spi_qmspi_dev_data_0, ctx),
};

DEVICE_AND_API_INIT(spi_qmspi_0, CONFIG_SPI_0_NAME, &spi_qmspi_init,
		    &spi_qmspi_dev_data_0, &spi_qmspi_cfg_0,
		    POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,
		    &api_funcs);

#endif
