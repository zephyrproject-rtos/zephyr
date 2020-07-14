/*
 * Copyright (c) 2018, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_lpspi

#include <errno.h>
#include <drivers/spi.h>
#include <drivers/clock_control.h>
#include <fsl_lpspi.h>

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(spi_mcux_lpspi);

#include "spi_context.h"

#define CHIP_SELECT_COUNT	4
#define MAX_DATA_WIDTH		4096

struct spi_mcux_config {
	LPSPI_Type *base;
	char *clock_name;
	clock_control_subsys_t clock_subsys;
	void (*irq_config_func)(const struct device *dev);
	uint32_t pcs_sck_delay;
	uint32_t sck_pcs_delay;
	uint32_t transfer_delay;
};

struct spi_mcux_data {
	const struct device *dev;
	lpspi_master_handle_t handle;
	struct spi_context ctx;
	size_t transfer_len;
};

static void spi_mcux_transfer_next_packet(const struct device *dev)
{
	const struct spi_mcux_config *config = dev->config;
	struct spi_mcux_data *data = dev->data;
	LPSPI_Type *base = config->base;
	struct spi_context *ctx = &data->ctx;
	lpspi_transfer_t transfer;
	status_t status;

	if ((ctx->tx_len == 0) && (ctx->rx_len == 0)) {
		/* nothing left to rx or tx, we're done! */
		spi_context_cs_control(&data->ctx, false);
		spi_context_complete(&data->ctx, 0);
		return;
	}

	transfer.configFlags = kLPSPI_MasterPcsContinuous |
			       (ctx->config->slave << LPSPI_MASTER_PCS_SHIFT);

	if (ctx->tx_len == 0) {
		/* rx only, nothing to tx */
		transfer.txData = NULL;
		transfer.rxData = ctx->rx_buf;
		transfer.dataSize = ctx->rx_len;
	} else if (ctx->rx_len == 0) {
		/* tx only, nothing to rx */
		transfer.txData = (uint8_t *) ctx->tx_buf;
		transfer.rxData = NULL;
		transfer.dataSize = ctx->tx_len;
	} else if (ctx->tx_len == ctx->rx_len) {
		/* rx and tx are the same length */
		transfer.txData = (uint8_t *) ctx->tx_buf;
		transfer.rxData = ctx->rx_buf;
		transfer.dataSize = ctx->tx_len;
	} else if (ctx->tx_len > ctx->rx_len) {
		/* Break up the tx into multiple transfers so we don't have to
		 * rx into a longer intermediate buffer. Leave chip select
		 * active between transfers.
		 */
		transfer.txData = (uint8_t *) ctx->tx_buf;
		transfer.rxData = ctx->rx_buf;
		transfer.dataSize = ctx->rx_len;
		transfer.configFlags |= kLPSPI_MasterPcsContinuous;
	} else {
		/* Break up the rx into multiple transfers so we don't have to
		 * tx from a longer intermediate buffer. Leave chip select
		 * active between transfers.
		 */
		transfer.txData = (uint8_t *) ctx->tx_buf;
		transfer.rxData = ctx->rx_buf;
		transfer.dataSize = ctx->tx_len;
		transfer.configFlags |= kLPSPI_MasterPcsContinuous;
	}

	if (!(ctx->tx_count <= 1 && ctx->rx_count <= 1)) {
		transfer.configFlags |= kLPSPI_MasterPcsContinuous;
	}

	data->transfer_len = transfer.dataSize;

	status = LPSPI_MasterTransferNonBlocking(base, &data->handle,
						 &transfer);
	if (status != kStatus_Success) {
		LOG_ERR("Transfer could not start");
	}
}

static void spi_mcux_isr(const struct device *dev)
{
	const struct spi_mcux_config *config = dev->config;
	struct spi_mcux_data *data = dev->data;
	LPSPI_Type *base = config->base;

	LPSPI_MasterTransferHandleIRQ(base, &data->handle);
}

static void spi_mcux_master_transfer_callback(LPSPI_Type *base,
		lpspi_master_handle_t *handle, status_t status, void *userData)
{
	struct spi_mcux_data *data = userData;

	spi_context_update_tx(&data->ctx, 1, data->transfer_len);
	spi_context_update_rx(&data->ctx, 1, data->transfer_len);

	spi_mcux_transfer_next_packet(data->dev);
}

static int spi_mcux_configure(const struct device *dev,
			      const struct spi_config *spi_cfg)
{
	const struct spi_mcux_config *config = dev->config;
	struct spi_mcux_data *data = dev->data;
	LPSPI_Type *base = config->base;
	lpspi_master_config_t master_config;
	const struct device *clock_dev;
	uint32_t clock_freq;
	uint32_t word_size;

	if (spi_context_configured(&data->ctx, spi_cfg)) {
		/* This configuration is already in use */
		return 0;
	}

	LPSPI_MasterGetDefaultConfig(&master_config);

	if (spi_cfg->slave > CHIP_SELECT_COUNT) {
		LOG_ERR("Slave %d is greater than %d",
			    spi_cfg->slave,
			    CHIP_SELECT_COUNT);
		return -EINVAL;
	}

	word_size = SPI_WORD_SIZE_GET(spi_cfg->operation);
	if (word_size > MAX_DATA_WIDTH) {
		LOG_ERR("Word size %d is greater than %d",
			    word_size, MAX_DATA_WIDTH);
		return -EINVAL;
	}

	master_config.bitsPerFrame = word_size;

	master_config.cpol =
		(SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPOL)
		? kLPSPI_ClockPolarityActiveLow
		: kLPSPI_ClockPolarityActiveHigh;

	master_config.cpha =
		(SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPHA)
		? kLPSPI_ClockPhaseSecondEdge
		: kLPSPI_ClockPhaseFirstEdge;

	master_config.direction =
		(spi_cfg->operation & SPI_TRANSFER_LSB)
		? kLPSPI_LsbFirst
		: kLPSPI_MsbFirst;

	master_config.baudRate = spi_cfg->frequency;

	master_config.pcsToSckDelayInNanoSec = config->pcs_sck_delay;
	master_config.lastSckToPcsDelayInNanoSec = config->sck_pcs_delay;
	master_config.betweenTransferDelayInNanoSec = config->transfer_delay;

	clock_dev = device_get_binding(config->clock_name);
	if (clock_dev == NULL) {
		return -EINVAL;
	}

	if (clock_control_get_rate(clock_dev, config->clock_subsys,
				   &clock_freq)) {
		return -EINVAL;
	}

	LPSPI_MasterInit(base, &master_config, clock_freq);

	LPSPI_MasterTransferCreateHandle(base, &data->handle,
					 spi_mcux_master_transfer_callback,
					 data);

	LPSPI_SetDummyData(base, 0);

	data->ctx.config = spi_cfg;
	spi_context_cs_configure(&data->ctx);

	return 0;
}

static int transceive(const struct device *dev,
		      const struct spi_config *spi_cfg,
		      const struct spi_buf_set *tx_bufs,
		      const struct spi_buf_set *rx_bufs,
		      bool asynchronous,
		      struct k_poll_signal *signal)
{
	struct spi_mcux_data *data = dev->data;
	int ret;

	spi_context_lock(&data->ctx, asynchronous, signal);

	ret = spi_mcux_configure(dev, spi_cfg);
	if (ret) {
		goto out;
	}

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

	spi_context_cs_control(&data->ctx, true);

	spi_mcux_transfer_next_packet(dev);

	ret = spi_context_wait_for_completion(&data->ctx);
out:
	spi_context_release(&data->ctx, ret);

	return ret;
}

static int spi_mcux_transceive(const struct device *dev,
			       const struct spi_config *spi_cfg,
			       const struct spi_buf_set *tx_bufs,
			       const struct spi_buf_set *rx_bufs)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, false, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int spi_mcux_transceive_async(const struct device *dev,
				     const struct spi_config *spi_cfg,
				     const struct spi_buf_set *tx_bufs,
				     const struct spi_buf_set *rx_bufs,
				     struct k_poll_signal *async)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, true, async);
}
#endif /* CONFIG_SPI_ASYNC */

static int spi_mcux_release(const struct device *dev,
			    const struct spi_config *spi_cfg)
{
	struct spi_mcux_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static int spi_mcux_init(const struct device *dev)
{
	const struct spi_mcux_config *config = dev->config;
	struct spi_mcux_data *data = dev->data;

	config->irq_config_func(dev);

	spi_context_unlock_unconditionally(&data->ctx);

	data->dev = dev;

	return 0;
}

static const struct spi_driver_api spi_mcux_driver_api = {
	.transceive = spi_mcux_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_mcux_transceive_async,
#endif
	.release = spi_mcux_release,
};

#define SPI_MCUX_LPSPI_INIT(n)						\
	static void spi_mcux_config_func_##n(const struct device *dev);	\
									\
	static const struct spi_mcux_config spi_mcux_config_##n = {	\
		.base = (LPSPI_Type *) DT_INST_REG_ADDR(n),		\
		.clock_name = DT_INST_CLOCKS_LABEL(n),			\
		.clock_subsys =						\
		(clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),	\
		.irq_config_func = spi_mcux_config_func_##n,		\
		.pcs_sck_delay = UTIL_AND(				\
			DT_INST_NODE_HAS_PROP(n, pcs_sck_delay),	\
			DT_INST_PROP(n, pcs_sck_delay)),		\
		.sck_pcs_delay = UTIL_AND(				\
			DT_INST_NODE_HAS_PROP(n, sck_pcs_delay),	\
			DT_INST_PROP(n, sck_pcs_delay)),		\
		.transfer_delay = UTIL_AND(				\
			DT_INST_NODE_HAS_PROP(n, transfer_delay),	\
			DT_INST_PROP(n, transfer_delay)),		\
	};								\
									\
	static struct spi_mcux_data spi_mcux_data_##n = {		\
		SPI_CONTEXT_INIT_LOCK(spi_mcux_data_##n, ctx),		\
		SPI_CONTEXT_INIT_SYNC(spi_mcux_data_##n, ctx),		\
	};								\
									\
	DEVICE_AND_API_INIT(spi_mcux_##n, DT_INST_LABEL(n),		\
			    &spi_mcux_init, &spi_mcux_data_##n,		\
			    &spi_mcux_config_##n, POST_KERNEL,		\
			    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			    &spi_mcux_driver_api);			\
									\
	static void spi_mcux_config_func_##n(const struct device *dev)	\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),	\
			    spi_mcux_isr, DEVICE_GET(spi_mcux_##n), 0);	\
									\
		irq_enable(DT_INST_IRQN(n));				\
	}

DT_INST_FOREACH_STATUS_OKAY(SPI_MCUX_LPSPI_INIT)
