/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright (c) 2017,2019, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpc_spi

#include <errno.h>
#include <drivers/spi.h>
#include <fsl_spi.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(spi_mcux_flexcomm, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"

#define SPI_CHIP_SELECT_COUNT	4
#define SPI_MAX_DATA_WIDTH	16

struct spi_mcux_config {
	SPI_Type *base;
	void (*irq_config_func)(const struct device *dev);
};

struct spi_mcux_data {
	const struct device *dev;
	spi_master_handle_t handle;
	struct spi_context ctx;
	size_t transfer_len;
};

static void spi_mcux_transfer_next_packet(const struct device *dev)
{
	const struct spi_mcux_config *config = dev->config;
	struct spi_mcux_data *data = dev->data;
	SPI_Type *base = config->base;
	struct spi_context *ctx = &data->ctx;
	spi_transfer_t transfer;
	status_t status;

	if ((ctx->tx_len == 0) && (ctx->rx_len == 0)) {
		/* nothing left to rx or tx, we're done! */
		spi_context_cs_control(&data->ctx, false);
		spi_context_complete(&data->ctx, 0);
		return;
	}

	transfer.configFlags = 0;
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
	} else {
		/* Break up the rx into multiple transfers so we don't have to
		 * tx from a longer intermediate buffer. Leave chip select
		 * active between transfers.
		 */
		transfer.txData = (uint8_t *) ctx->tx_buf;
		transfer.rxData = ctx->rx_buf;
		transfer.dataSize = ctx->tx_len;
	}

	if (ctx->tx_count <= 1 && ctx->rx_count <= 1) {
		transfer.configFlags = kSPI_FrameAssert;
	}

	data->transfer_len = transfer.dataSize;

	status = SPI_MasterTransferNonBlocking(base, &data->handle, &transfer);
	if (status != kStatus_Success) {
		LOG_ERR("Transfer could not start");
	}
}

static void spi_mcux_isr(const struct device *dev)
{
	const struct spi_mcux_config *config = dev->config;
	struct spi_mcux_data *data = dev->data;
	SPI_Type *base = config->base;

	SPI_MasterTransferHandleIRQ(base, &data->handle);
}

static void spi_mcux_transfer_callback(SPI_Type *base,
		spi_master_handle_t *handle, status_t status, void *userData)
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
	SPI_Type *base = config->base;
	uint32_t clock_freq;
	uint32_t word_size;

	if (spi_context_configured(&data->ctx, spi_cfg)) {
		/* This configuration is already in use */
		return 0;
	}

	word_size = SPI_WORD_SIZE_GET(spi_cfg->operation);
	if (word_size > SPI_MAX_DATA_WIDTH) {
		LOG_ERR("Word size %d is greater than %d",
			    word_size, SPI_MAX_DATA_WIDTH);
		return -EINVAL;
	}

	/*
	 * Do master or slave initializastion, depending on the
	 * mode requested.
	 */
	if (SPI_OP_MODE_GET(spi_cfg->operation) == SPI_OP_MODE_MASTER) {
		spi_master_config_t master_config;

		SPI_MasterGetDefaultConfig(&master_config);

		if (spi_cfg->slave > SPI_CHIP_SELECT_COUNT) {
			LOG_ERR("Slave %d is greater than %d",
				    spi_cfg->slave, SPI_CHIP_SELECT_COUNT);
			return -EINVAL;
		}

		master_config.sselNum = spi_cfg->slave;
		master_config.sselPol = kSPI_SpolActiveAllLow;
		master_config.dataWidth = word_size - 1;

		master_config.polarity =
			(SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPOL)
			? kSPI_ClockPolarityActiveLow
			: kSPI_ClockPolarityActiveHigh;

		master_config.phase =
			(SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPHA)
			? kSPI_ClockPhaseSecondEdge
			: kSPI_ClockPhaseFirstEdge;

		master_config.direction =
			(spi_cfg->operation & SPI_TRANSFER_LSB)
			? kSPI_LsbFirst
			: kSPI_MsbFirst;

		master_config.baudRate_Bps = spi_cfg->frequency;

		/* The clock frequency is hardcoded CPU's speed to allow SPI to
		 * function at high speeds. The core clock and flexcomm should
		 * use the same clock source.
		 */
		clock_freq = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;

		SPI_MasterInit(base, &master_config, clock_freq);

		SPI_MasterTransferCreateHandle(base, &data->handle,
					       spi_mcux_transfer_callback, data);

		SPI_SetDummyData(base, 0);

		spi_context_cs_configure(&data->ctx);
	} else {
		spi_slave_config_t slave_config;

		SPI_SlaveGetDefaultConfig(&slave_config);

		slave_config.polarity =
			(SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPOL)
			? kSPI_ClockPolarityActiveLow
			: kSPI_ClockPolarityActiveHigh;

		slave_config.phase =
			(SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPHA)
			? kSPI_ClockPhaseSecondEdge
			: kSPI_ClockPhaseFirstEdge;

		slave_config.direction =
			(spi_cfg->operation & SPI_TRANSFER_LSB)
			? kSPI_LsbFirst
			: kSPI_MsbFirst;

		/* SS pin active low */
		slave_config.sselPol = kSPI_SpolActiveAllLow;
		slave_config.dataWidth = word_size - 1;

		SPI_SlaveInit(base, &slave_config);

		SPI_SlaveTransferCreateHandle(base, &data->handle,
					      spi_mcux_transfer_callback, data);
	}

	data->ctx.config = spi_cfg;

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

	data->dev = dev;

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static const struct spi_driver_api spi_mcux_driver_api = {
	.transceive = spi_mcux_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_mcux_transceive_async,
#endif
	.release = spi_mcux_release,
};

#define SPI_MCUX_FLEXCOMM_DEVICE(id)					\
	static void spi_mcux_config_func_##id(const struct device *dev); \
	static const struct spi_mcux_config spi_mcux_config_##id = {	\
		.base =							\
		(SPI_Type *)DT_INST_REG_ADDR(id),			\
		.irq_config_func = spi_mcux_config_func_##id,		\
	};								\
	static struct spi_mcux_data spi_mcux_data_##id = {		\
		SPI_CONTEXT_INIT_LOCK(spi_mcux_data_##id, ctx),		\
		SPI_CONTEXT_INIT_SYNC(spi_mcux_data_##id, ctx),		\
	};								\
	DEVICE_AND_API_INIT(spi_mcux_##id,				\
			    DT_INST_LABEL(id),				\
			    &spi_mcux_init,				\
			    &spi_mcux_data_##id,			\
			    &spi_mcux_config_##id,			\
			    POST_KERNEL,				\
			    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			    &spi_mcux_driver_api);			\
	static void spi_mcux_config_func_##id(const struct device *dev)	\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(id),				\
			    DT_INST_IRQ(id, priority),			\
			    spi_mcux_isr, DEVICE_GET(spi_mcux_##id),	\
			    0);						\
		irq_enable(DT_INST_IRQN(id));				\
	}

DT_INST_FOREACH_STATUS_OKAY(SPI_MCUX_FLEXCOMM_DEVICE)
