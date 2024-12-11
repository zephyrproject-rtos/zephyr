/*
 * Copyright 2018, 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpspi

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_mcux_lpspi, CONFIG_SPI_LOG_LEVEL);

#include "spi_nxp_lpspi_priv.h"

struct lpspi_driver_data {
	lpspi_master_handle_t handle;
};

static int spi_mcux_transfer_next_packet(const struct device *dev)
{
	struct spi_mcux_data *data = dev->data;
	struct lpspi_driver_data *lpspi_data = (struct lpspi_driver_data *)data->driver_data;
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	struct spi_context *ctx = &data->ctx;
	size_t max_chunk = spi_context_max_continuous_chunk(ctx);
	lpspi_transfer_t transfer;
	status_t status;

	if (max_chunk == 0) {
		spi_context_cs_control(ctx, false);
		spi_context_complete(ctx, dev, 0);
		return 0;
	}

	data->transfer_len = max_chunk;

	transfer.configFlags = LPSPI_MASTER_XFER_CFG_FLAGS(ctx->config->slave);
	transfer.txData = (ctx->tx_len == 0 ? NULL : ctx->tx_buf);
	transfer.rxData = (ctx->rx_len == 0 ? NULL : ctx->rx_buf);
	transfer.dataSize = max_chunk;

	status = LPSPI_MasterTransferNonBlocking(base, &lpspi_data->handle, &transfer);
	if (status != kStatus_Success) {
		LOG_ERR("Transfer could not start on %s: %d", dev->name, status);
		return status == kStatus_LPSPI_Busy ? -EBUSY : -EINVAL;
	}

	return 0;
}

static void lpspi_isr(const struct device *dev)
{
	struct spi_mcux_data *data = dev->data;
	struct lpspi_driver_data *lpspi_data = (struct lpspi_driver_data *)data->driver_data;
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);

	LPSPI_MasterTransferHandleIRQ(LPSPI_IRQ_HANDLE_ARG, &lpspi_data->handle);
}

static void spi_mcux_master_callback(LPSPI_Type *base, lpspi_master_handle_t *handle,
				     status_t status, void *userData)
{
	struct spi_mcux_data *data = userData;

	spi_context_update_tx(&data->ctx, 1, data->transfer_len);
	spi_context_update_rx(&data->ctx, 1, data->transfer_len);

	spi_mcux_transfer_next_packet(data->dev);
}

static int transceive(const struct device *dev, const struct spi_config *spi_cfg,
		      const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs,
		      bool asynchronous, spi_callback_t cb, void *userdata)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	struct spi_mcux_data *data = dev->data;
	struct lpspi_driver_data *lpspi_data = (struct lpspi_driver_data *)data->driver_data;
	int ret;

	spi_context_lock(&data->ctx, asynchronous, cb, userdata, spi_cfg);

	ret = spi_mcux_configure(dev, spi_cfg);
	if (ret) {
		goto out;
	}

	LPSPI_MasterTransferCreateHandle(base, &lpspi_data->handle, spi_mcux_master_callback, data);

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

	spi_context_cs_control(&data->ctx, true);

	ret = spi_mcux_transfer_next_packet(dev);
	if (ret) {
		goto out;
	}

	ret = spi_context_wait_for_completion(&data->ctx);
out:
	spi_context_release(&data->ctx, ret);

	return ret;
}

static int spi_mcux_transceive_sync(const struct device *dev, const struct spi_config *spi_cfg,
				    const struct spi_buf_set *tx_bufs,
				    const struct spi_buf_set *rx_bufs)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, false, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int spi_mcux_transceive_async(const struct device *dev, const struct spi_config *spi_cfg,
				     const struct spi_buf_set *tx_bufs,
				     const struct spi_buf_set *rx_bufs, spi_callback_t cb,
				     void *userdata)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, true, cb, userdata);
}
#endif /* CONFIG_SPI_ASYNC */

static DEVICE_API(spi, spi_mcux_driver_api) = {
	.transceive = spi_mcux_transceive_sync,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_mcux_transceive_async,
#endif
	.release = spi_mcux_release,
};

static int spi_mcux_init(const struct device *dev)
{
	struct spi_mcux_data *data = dev->data;
	int err = 0;

	err = spi_nxp_init_common(dev);
	if (err) {
		return err;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#define LPSPI_INIT(n)                                                                              \
	SPI_NXP_LPSPI_COMMON_INIT(n)                                                               \
	SPI_MCUX_LPSPI_CONFIG_INIT(n)                                                              \
                                                                                                   \
	static struct lpspi_driver_data lpspi_##n##_driver_data;                                   \
                                                                                                   \
	static struct spi_mcux_data spi_mcux_data_##n = {                                          \
		SPI_NXP_LPSPI_COMMON_DATA_INIT(n)                                                  \
		.driver_data = &lpspi_##n##_driver_data,                                           \
	};                                                                                         \
                                                                                                   \
	SPI_DEVICE_DT_INST_DEFINE(n, spi_mcux_init, NULL, &spi_mcux_data_##n,                      \
				  &spi_mcux_config_##n, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,     \
				  &spi_mcux_driver_api);

#define SPI_MCUX_LPSPI_INIT_IF_DMA(n) IF_DISABLED(SPI_NXP_LPSPI_HAS_DMAS(n), (LPSPI_INIT(n)))

#define SPI_MCUX_LPSPI_INIT(n)                                                                     \
	COND_CODE_1(CONFIG_SPI_MCUX_LPSPI_DMA,				   \
						(SPI_MCUX_LPSPI_INIT_IF_DMA(n)), (LPSPI_INIT(n)))

DT_INST_FOREACH_STATUS_OKAY(SPI_MCUX_LPSPI_INIT)
