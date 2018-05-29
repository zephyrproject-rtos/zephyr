/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <spi.h>
#include <clock_control.h>
#include <fsl_dspi.h>

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SPI_LEVEL
#include <logging/sys_log.h>

#include "spi_context.h"

struct spi_mcux_config {
	SPI_Type *base;
	char *clock_name;
	clock_control_subsys_t clock_subsys;
	void (*irq_config_func)(struct device *dev);
};

struct spi_mcux_data {
	dspi_master_handle_t handle;
	struct spi_context ctx;
	size_t transfer_len;
};

static void spi_mcux_transfer_next_packet(struct device *dev)
{
	const struct spi_mcux_config *config = dev->config->config_info;
	struct spi_mcux_data *data = dev->driver_data;
	SPI_Type *base = config->base;
	struct spi_context *ctx = &data->ctx;
	dspi_transfer_t transfer;
	status_t status;

	if ((ctx->tx_len == 0) && (ctx->rx_len == 0)) {
		/* nothing left to rx or tx, we're done! */
		spi_context_cs_control(&data->ctx, false);
		spi_context_complete(&data->ctx, 0);
		return;
	}

	transfer.configFlags = kDSPI_MasterCtar0 | kDSPI_MasterPcsContinuous |
			       (ctx->config->slave << DSPI_MASTER_PCS_SHIFT);

	if (ctx->tx_len == 0) {
		/* rx only, nothing to tx */
		transfer.txData = NULL;
		transfer.rxData = ctx->rx_buf;
		transfer.dataSize = ctx->rx_len;
	} else if (ctx->rx_len == 0) {
		/* tx only, nothing to rx */
		transfer.txData = (u8_t *) ctx->tx_buf;
		transfer.rxData = NULL;
		transfer.dataSize = ctx->tx_len;
	} else if (ctx->tx_len == ctx->rx_len) {
		/* rx and tx are the same length */
		transfer.txData = (u8_t *) ctx->tx_buf;
		transfer.rxData = ctx->rx_buf;
		transfer.dataSize = ctx->tx_len;
	} else if (ctx->tx_len > ctx->rx_len) {
		/* Break up the tx into multiple transfers so we don't have to
		 * rx into a longer intermediate buffer. Leave chip select
		 * active between transfers.
		 */
		transfer.txData = (u8_t *) ctx->tx_buf;
		transfer.rxData = ctx->rx_buf;
		transfer.dataSize = ctx->rx_len;
		transfer.configFlags |= kDSPI_MasterActiveAfterTransfer;
	} else {
		/* Break up the rx into multiple transfers so we don't have to
		 * tx from a longer intermediate buffer. Leave chip select
		 * active between transfers.
		 */
		transfer.txData = (u8_t *) ctx->tx_buf;
		transfer.rxData = ctx->rx_buf;
		transfer.dataSize = ctx->tx_len;
		transfer.configFlags |= kDSPI_MasterActiveAfterTransfer;
	}

	if (!(ctx->tx_count <= 1 && ctx->rx_count <= 1)) {
		transfer.configFlags |= kDSPI_MasterActiveAfterTransfer;
	}

	data->transfer_len = transfer.dataSize;

	status = DSPI_MasterTransferNonBlocking(base, &data->handle, &transfer);
	if (status != kStatus_Success) {
		SYS_LOG_ERR("Transfer could not start");
	}
}

static void spi_mcux_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct spi_mcux_config *config = dev->config->config_info;
	struct spi_mcux_data *data = dev->driver_data;
	SPI_Type *base = config->base;

	DSPI_MasterTransferHandleIRQ(base, &data->handle);
}

static void spi_mcux_master_transfer_callback(SPI_Type *base,
		dspi_master_handle_t *handle, status_t status, void *userData)
{
	struct device *dev = userData;
	struct spi_mcux_data *data = dev->driver_data;

	spi_context_update_tx(&data->ctx, 1, data->transfer_len);
	spi_context_update_rx(&data->ctx, 1, data->transfer_len);

	spi_mcux_transfer_next_packet(dev);
}

static int spi_mcux_configure(struct device *dev,
			      const struct spi_config *spi_cfg)
{
	const struct spi_mcux_config *config = dev->config->config_info;
	struct spi_mcux_data *data = dev->driver_data;
	SPI_Type *base = config->base;
	dspi_master_config_t master_config;
	struct device *clock_dev;
	u32_t clock_freq;
	u32_t word_size;

	DSPI_MasterGetDefaultConfig(&master_config);

	if (spi_cfg->slave > FSL_FEATURE_DSPI_CHIP_SELECT_COUNT) {
		SYS_LOG_ERR("Slave %d is greater than %d",
			    spi_cfg->slave, FSL_FEATURE_DSPI_CHIP_SELECT_COUNT);
		return -EINVAL;
	}

	word_size = SPI_WORD_SIZE_GET(spi_cfg->operation);
	if (word_size > FSL_FEATURE_DSPI_MAX_DATA_WIDTH) {
		SYS_LOG_ERR("Word size %d is greater than %d",
			    word_size, FSL_FEATURE_DSPI_MAX_DATA_WIDTH);
		return -EINVAL;
	}

	master_config.ctarConfig.bitsPerFrame = word_size;

	master_config.ctarConfig.cpol =
		(SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPOL)
		? kDSPI_ClockPolarityActiveLow
		: kDSPI_ClockPolarityActiveHigh;

	master_config.ctarConfig.cpha =
		(SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPHA)
		? kDSPI_ClockPhaseSecondEdge
		: kDSPI_ClockPhaseFirstEdge;

	master_config.ctarConfig.direction =
		(spi_cfg->operation & SPI_TRANSFER_LSB)
		? kDSPI_LsbFirst
		: kDSPI_MsbFirst;

	master_config.ctarConfig.baudRate = spi_cfg->frequency;

	clock_dev = device_get_binding(config->clock_name);
	if (clock_dev == NULL) {
		return -EINVAL;
	}

	if (clock_control_get_rate(clock_dev, config->clock_subsys,
				   &clock_freq)) {
		return -EINVAL;
	}

	DSPI_MasterInit(base, &master_config, clock_freq);

	DSPI_MasterTransferCreateHandle(base, &data->handle,
					spi_mcux_master_transfer_callback, dev);

	data->ctx.config = spi_cfg;
	spi_context_cs_configure(&data->ctx);

	return 0;
}

static int transceive(struct device *dev,
		      const struct spi_config *spi_cfg,
		      const struct spi_buf_set *tx_bufs,
		      const struct spi_buf_set *rx_bufs,
		      bool asynchronous,
		      struct k_poll_signal *signal)
{
	struct spi_mcux_data *data = dev->driver_data;
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

static int spi_mcux_transceive(struct device *dev,
			       const struct spi_config *spi_cfg,
			       const struct spi_buf_set *tx_bufs,
			       const struct spi_buf_set *rx_bufs)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, false, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int spi_mcux_transceive_async(struct device *dev,
				     const struct spi_config *spi_cfg,
				     const struct spi_buf_set *tx_bufs,
				     const struct spi_buf_set *rx_bufs,
				     struct k_poll_signal *async)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, true, async);
}
#endif /* CONFIG_SPI_ASYNC */

static int spi_mcux_release(struct device *dev,
		      const struct spi_config *spi_cfg)
{
	struct spi_mcux_data *data = dev->driver_data;

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static int spi_mcux_init(struct device *dev)
{
	const struct spi_mcux_config *config = dev->config->config_info;
	struct spi_mcux_data *data = dev->driver_data;

	config->irq_config_func(dev);

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

#ifdef CONFIG_SPI_0
static void spi_mcux_config_func_0(struct device *dev);

static const struct spi_mcux_config spi_mcux_config_0 = {
	.base = (SPI_Type *) CONFIG_SPI_0_BASE_ADDRESS,
	.clock_name = CONFIG_SPI_0_CLOCK_NAME,
	.clock_subsys = (clock_control_subsys_t) CONFIG_SPI_0_CLOCK_SUBSYS,
	.irq_config_func = spi_mcux_config_func_0,
};

static struct spi_mcux_data spi_mcux_data_0 = {
	SPI_CONTEXT_INIT_LOCK(spi_mcux_data_0, ctx),
	SPI_CONTEXT_INIT_SYNC(spi_mcux_data_0, ctx),
};

DEVICE_AND_API_INIT(spi_mcux_0, CONFIG_SPI_0_NAME, &spi_mcux_init,
		    &spi_mcux_data_0, &spi_mcux_config_0,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &spi_mcux_driver_api);

static void spi_mcux_config_func_0(struct device *dev)
{
	IRQ_CONNECT(CONFIG_SPI_0_IRQ, CONFIG_SPI_0_IRQ_PRI,
		    spi_mcux_isr, DEVICE_GET(spi_mcux_0), 0);

	irq_enable(CONFIG_SPI_0_IRQ);
}
#endif /* CONFIG_SPI_0 */

#ifdef CONFIG_SPI_1
static void spi_mcux_config_func_1(struct device *dev);

static const struct spi_mcux_config spi_mcux_config_1 = {
	.base = (SPI_Type *) CONFIG_SPI_1_BASE_ADDRESS,
	.clock_name = CONFIG_SPI_1_CLOCK_NAME,
	.clock_subsys = (clock_control_subsys_t) CONFIG_SPI_1_CLOCK_SUBSYS,
	.irq_config_func = spi_mcux_config_func_1,
};

static struct spi_mcux_data spi_mcux_data_1 = {
	SPI_CONTEXT_INIT_LOCK(spi_mcux_data_1, ctx),
	SPI_CONTEXT_INIT_SYNC(spi_mcux_data_1, ctx),
};

DEVICE_AND_API_INIT(spi_mcux_1, CONFIG_SPI_1_NAME, &spi_mcux_init,
		    &spi_mcux_data_1, &spi_mcux_config_1,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &spi_mcux_driver_api);

static void spi_mcux_config_func_1(struct device *dev)
{
	IRQ_CONNECT(CONFIG_SPI_1_IRQ, CONFIG_SPI_1_IRQ_PRI,
		    spi_mcux_isr, DEVICE_GET(spi_mcux_1), 0);

	irq_enable(CONFIG_SPI_1_IRQ);
}
#endif /* CONFIG_SPI_1 */

#ifdef CONFIG_SPI_2
static void spi_mcux_config_func_2(struct device *dev);

static const struct spi_mcux_config spi_mcux_config_2 = {
	.base = (SPI_Type *) CONFIG_SPI_2_BASE_ADDRESS,
	.clock_name = CONFIG_SPI_2_CLOCK_NAME,
	.clock_subsys = (clock_control_subsys_t) CONFIG_SPI_2_CLOCK_SUBSYS,
	.irq_config_func = spi_mcux_config_func_2,
};

static struct spi_mcux_data spi_mcux_data_2 = {
	SPI_CONTEXT_INIT_LOCK(spi_mcux_data_2, ctx),
	SPI_CONTEXT_INIT_SYNC(spi_mcux_data_2, ctx),
};

DEVICE_AND_API_INIT(spi_mcux_2, CONFIG_SPI_2_NAME, &spi_mcux_init,
		    &spi_mcux_data_2, &spi_mcux_config_2,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &spi_mcux_driver_api);

static void spi_mcux_config_func_2(struct device *dev)
{
	IRQ_CONNECT(CONFIG_SPI_2_IRQ, CONFIG_SPI_2_IRQ_PRI,
		    spi_mcux_isr, DEVICE_GET(spi_mcux_2), 0);

	irq_enable(CONFIG_SPI_2_IRQ);
}
#endif /* CONFIG_SPI_2 */
