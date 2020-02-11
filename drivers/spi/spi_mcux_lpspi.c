/*
 * Copyright (c) 2018, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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
	void (*irq_config_func)(struct device *dev);
};

struct spi_mcux_data {
	lpspi_master_handle_t handle;
	struct spi_context ctx;
	size_t transfer_len;
};

static void spi_mcux_transfer_next_packet(struct device *dev)
{
	const struct spi_mcux_config *config = dev->config->config_info;
	struct spi_mcux_data *data = dev->driver_data;
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
		transfer.configFlags |= kLPSPI_MasterPcsContinuous;
	} else {
		/* Break up the rx into multiple transfers so we don't have to
		 * tx from a longer intermediate buffer. Leave chip select
		 * active between transfers.
		 */
		transfer.txData = (u8_t *) ctx->tx_buf;
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

static void spi_mcux_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct spi_mcux_config *config = dev->config->config_info;
	struct spi_mcux_data *data = dev->driver_data;
	LPSPI_Type *base = config->base;

	LPSPI_MasterTransferHandleIRQ(base, &data->handle);
}

static void spi_mcux_master_transfer_callback(LPSPI_Type *base,
		lpspi_master_handle_t *handle, status_t status, void *userData)
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
	LPSPI_Type *base = config->base;
	lpspi_master_config_t master_config;
	struct device *clock_dev;
	u32_t clock_freq;
	u32_t word_size;

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
	.base = (LPSPI_Type *) DT_ALIAS_SPI_0_BASE_ADDRESS,
	.clock_name = DT_ALIAS_SPI_0_CLOCK_CONTROLLER,
	.clock_subsys = (clock_control_subsys_t) DT_ALIAS_SPI_0_CLOCK_NAME,
	.irq_config_func = spi_mcux_config_func_0,
};

static struct spi_mcux_data spi_mcux_data_0 = {
	SPI_CONTEXT_INIT_LOCK(spi_mcux_data_0, ctx),
	SPI_CONTEXT_INIT_SYNC(spi_mcux_data_0, ctx),
};

DEVICE_AND_API_INIT(spi_mcux_0, DT_ALIAS_SPI_0_LABEL, &spi_mcux_init,
		    &spi_mcux_data_0, &spi_mcux_config_0,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &spi_mcux_driver_api);

static void spi_mcux_config_func_0(struct device *dev)
{
	IRQ_CONNECT(DT_ALIAS_SPI_0_IRQ_0, DT_ALIAS_SPI_0_IRQ_0_PRIORITY,
		    spi_mcux_isr, DEVICE_GET(spi_mcux_0), 0);

	irq_enable(DT_ALIAS_SPI_0_IRQ_0);
}
#endif /* SPI_0 */

#ifdef CONFIG_SPI_1
static void spi_mcux_config_func_1(struct device *dev);

static const struct spi_mcux_config spi_mcux_config_1 = {
	.base = (LPSPI_Type *) DT_ALIAS_SPI_1_BASE_ADDRESS,
	.clock_name = DT_ALIAS_SPI_1_CLOCK_CONTROLLER,
	.clock_subsys = (clock_control_subsys_t) DT_ALIAS_SPI_1_CLOCK_NAME,
	.irq_config_func = spi_mcux_config_func_1,
};

static struct spi_mcux_data spi_mcux_data_1 = {
	SPI_CONTEXT_INIT_LOCK(spi_mcux_data_1, ctx),
	SPI_CONTEXT_INIT_SYNC(spi_mcux_data_1, ctx),
};

DEVICE_AND_API_INIT(spi_mcux_1, DT_ALIAS_SPI_1_LABEL, &spi_mcux_init,
		    &spi_mcux_data_1, &spi_mcux_config_1,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &spi_mcux_driver_api);

static void spi_mcux_config_func_1(struct device *dev)
{
	IRQ_CONNECT(DT_ALIAS_SPI_1_IRQ_0, DT_ALIAS_SPI_1_IRQ_0_PRIORITY,
		    spi_mcux_isr, DEVICE_GET(spi_mcux_1), 0);

	irq_enable(DT_ALIAS_SPI_1_IRQ_0);
}
#endif /* SPI_1 */

#ifdef CONFIG_SPI_2
static void spi_mcux_config_func_2(struct device *dev);

static const struct spi_mcux_config spi_mcux_config_2 = {
	.base = (LPSPI_Type *) DT_ALIAS_SPI_2_BASE_ADDRESS,
	.clock_name = DT_ALIAS_SPI_2_CLOCK_CONTROLLER,
	.clock_subsys = (clock_control_subsys_t) DT_ALIAS_SPI_2_CLOCK_NAME,
	.irq_config_func = spi_mcux_config_func_2,
};

static struct spi_mcux_data spi_mcux_data_2 = {
	SPI_CONTEXT_INIT_LOCK(spi_mcux_data_2, ctx),
	SPI_CONTEXT_INIT_SYNC(spi_mcux_data_2, ctx),
};

DEVICE_AND_API_INIT(spi_mcux_2, DT_ALIAS_SPI_2_LABEL, &spi_mcux_init,
		    &spi_mcux_data_2, &spi_mcux_config_2,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &spi_mcux_driver_api);

static void spi_mcux_config_func_2(struct device *dev)
{
	IRQ_CONNECT(DT_ALIAS_SPI_2_IRQ_0, DT_ALIAS_SPI_2_IRQ_0_PRIORITY,
		    spi_mcux_isr, DEVICE_GET(spi_mcux_2), 0);

	irq_enable(DT_ALIAS_SPI_2_IRQ_0);
}
#endif /* SPI_2 */

#ifdef CONFIG_SPI_3
static void spi_mcux_config_func_3(struct device *dev);

static const struct spi_mcux_config spi_mcux_config_3 = {
	.base = (LPSPI_Type *) DT_ALIAS_SPI_3_BASE_ADDRESS,
	.clock_name = DT_ALIAS_SPI_3_CLOCK_CONTROLLER,
	.clock_subsys = (clock_control_subsys_t) DT_ALIAS_SPI_3_CLOCK_NAME,
	.irq_config_func = spi_mcux_config_func_3,
};

static struct spi_mcux_data spi_mcux_data_3 = {
	SPI_CONTEXT_INIT_LOCK(spi_mcux_data_3, ctx),
	SPI_CONTEXT_INIT_SYNC(spi_mcux_data_3, ctx),
};

DEVICE_AND_API_INIT(spi_mcux_3, DT_ALIAS_SPI_3_LABEL, &spi_mcux_init,
		    &spi_mcux_data_3, &spi_mcux_config_3,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &spi_mcux_driver_api);

static void spi_mcux_config_func_3(struct device *dev)
{
	IRQ_CONNECT(DT_ALIAS_SPI_3_IRQ_0, DT_ALIAS_SPI_3_IRQ_0_PRIORITY,
		    spi_mcux_isr, DEVICE_GET(spi_mcux_3), 0);

	irq_enable(DT_ALIAS_SPI_3_IRQ_0);
}
#endif /* CONFIG_SPI_3 */
