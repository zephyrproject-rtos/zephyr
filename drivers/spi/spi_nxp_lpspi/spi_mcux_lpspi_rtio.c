/*
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpspi

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_mcux_lpspi_rtio, CONFIG_SPI_LOG_LEVEL);

#include <zephyr/drivers/spi/rtio.h>
#include "spi_nxp_lpspi_priv.h"

static int spi_mcux_transfer_next_packet(const struct device *dev)
{
	struct spi_mcux_data *data = dev->data;
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

	status = LPSPI_MasterTransferNonBlocking(base, &data->handle, &transfer);
	if (status != kStatus_Success) {
		LOG_ERR("Transfer could not start on %s: %d", dev->name, status);
		return status == kStatus_LPSPI_Busy ? -EBUSY : -EINVAL;
	}

	return 0;
}

static void spi_mcux_iodev_complete(const struct device *dev, int status);

static void spi_mcux_master_rtio_callback(LPSPI_Type *base, lpspi_master_handle_t *handle,
					  status_t status, void *userData)
{
	struct spi_mcux_data *data = userData;
	struct spi_rtio *rtio_ctx = data->rtio_ctx;

	if (rtio_ctx->txn_head != NULL) {
		spi_mcux_iodev_complete(data->dev, status);
		return;
	}

	spi_context_update_tx(&data->ctx, 1, data->transfer_len);
	spi_context_update_rx(&data->ctx, 1, data->transfer_len);

	spi_mcux_transfer_next_packet(data->dev);
}

static void spi_mcux_iodev_start(const struct device *dev)
{
	struct spi_mcux_data *data = dev->data;
	struct spi_rtio *rtio_ctx = data->rtio_ctx;
	struct rtio_sqe *sqe = &rtio_ctx->txn_curr->sqe;
	struct spi_dt_spec *spi_dt_spec = sqe->iodev->data;
	struct spi_config *spi_cfg = &spi_dt_spec->config;
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	lpspi_transfer_t transfer;
	status_t status;

	status = spi_mcux_configure(dev, spi_cfg);
	if (status) {
		LOG_ERR("Error configuring lpspi");
		return;
	}

	LPSPI_MasterTransferCreateHandle(base, &data->handle, spi_mcux_master_rtio_callback, data);

	transfer.configFlags = LPSPI_MASTER_XFER_CFG_FLAGS(spi_cfg->slave);

	switch (sqe->op) {
	case RTIO_OP_RX:
		transfer.txData = NULL;
		transfer.rxData = sqe->rx.buf;
		transfer.dataSize = sqe->rx.buf_len;
		break;
	case RTIO_OP_TX:
		transfer.rxData = NULL;
		transfer.txData = sqe->tx.buf;
		transfer.dataSize = sqe->tx.buf_len;
		break;
	case RTIO_OP_TINY_TX:
		transfer.rxData = NULL;
		transfer.txData = sqe->tiny_tx.buf;
		transfer.dataSize = sqe->tiny_tx.buf_len;
		break;
	case RTIO_OP_TXRX:
		transfer.txData = sqe->txrx.tx_buf;
		transfer.rxData = sqe->txrx.rx_buf;
		transfer.dataSize = sqe->txrx.buf_len;
		break;
	default:
		LOG_ERR("Invalid op code %d for submission %p\n", sqe->op, (void *)sqe);
		spi_mcux_iodev_complete(dev, -EINVAL);
		return;
	}

	data->transfer_len = transfer.dataSize;

	spi_context_cs_control(&data->ctx, true);

	status = LPSPI_MasterTransferNonBlocking(base, &data->handle, &transfer);
	if (status != kStatus_Success) {
		LOG_ERR("Transfer could not start on %s: %d", dev->name, status);
		spi_mcux_iodev_complete(dev, -EIO);
	}
}

static void spi_mcux_iodev_complete(const struct device *dev, int status)
{
	struct spi_mcux_data *data = dev->data;
	struct spi_rtio *rtio_ctx = data->rtio_ctx;

	if (!status && rtio_ctx->txn_curr->sqe.flags & RTIO_SQE_TRANSACTION) {
		rtio_ctx->txn_curr = rtio_txn_next(rtio_ctx->txn_curr);
		spi_mcux_iodev_start(dev);
		return;
	}

	/** De-assert CS-line to space from next transaction */
	spi_context_cs_control(&data->ctx, false);

	if (spi_rtio_complete(rtio_ctx, status)) {
		spi_mcux_iodev_start(dev);
	}
}

static void spi_mcux_iodev_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct spi_mcux_data *data = dev->data;
	struct spi_rtio *rtio_ctx = data->rtio_ctx;

	if (spi_rtio_submit(rtio_ctx, iodev_sqe)) {
		spi_mcux_iodev_start(dev);
	}
}

static int transceive_rtio(const struct device *dev, const struct spi_config *spi_cfg,
			   const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs)
{
	struct spi_mcux_data *data = dev->data;
	struct spi_rtio *rtio_ctx = data->rtio_ctx;
	int ret;

	spi_context_lock(&data->ctx, false, NULL, NULL, spi_cfg);

	ret = spi_rtio_transceive(rtio_ctx, spi_cfg, tx_bufs, rx_bufs);

	spi_context_release(&data->ctx, ret);

	return ret;
}

#ifdef CONFIG_SPI_ASYNC
static int transceive_rtio_async(const struct device *dev, const struct spi_config *spi_cfg,
				 const struct spi_buf_set *tx_bufs,
				 const struct spi_buf_set *rx_bufs, spi_callback_t cb,
				 void *userdata)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(spi_cfg);
	ARG_UNUSED(tx_bufs);
	ARG_UNUSED(rx_bufs);
	ARG_UNUSED(cb);
	ARG_UNUSED(userdata);

	return -ENOTSUP;
}
#endif

static DEVICE_API(spi, spi_mcux_rtio_driver_api) = {
	.transceive = transceive_rtio,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = transceive_rtio_async,
#endif
	.iodev_submit = spi_mcux_iodev_submit,
	.release = spi_mcux_release,
};

static int spi_mcux_rtio_init(const struct device *dev)
{
	struct spi_mcux_data *data = dev->data;
	int err = 0;

	err = spi_nxp_init_common(dev);
	if (err) {
		return err;
	}

	spi_rtio_init(data->rtio_ctx, dev);

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static void lpspi_isr(const struct device *dev)
{
	struct spi_mcux_data *data = dev->data;
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);

	LPSPI_MasterTransferHandleIRQ(LPSPI_IRQ_HANDLE_ARG, &data->handle);
}

#define SPI_MCUX_RTIO_DEFINE(n)                                                                    \
	SPI_RTIO_DEFINE(spi_mcux_rtio_##n, CONFIG_SPI_MCUX_RTIO_SQ_SIZE,                           \
			CONFIG_SPI_MCUX_RTIO_SQ_SIZE)

#define SPI_MCUX_LPSPI_RTIO_INIT(n)                                                                \
	SPI_MCUX_RTIO_DEFINE(n);                                                                   \
	SPI_NXP_LPSPI_COMMON_INIT(n)                                                               \
	SPI_MCUX_LPSPI_CONFIG_INIT(n)                                                              \
                                                                                                   \
	static struct spi_mcux_data spi_mcux_data_##n = {.rtio_ctx = &spi_mcux_rtio_##n,           \
							 SPI_NXP_LPSPI_COMMON_DATA_INIT(n)};       \
                                                                                                   \
	SPI_DEVICE_DT_INST_DEFINE(n, spi_mcux_rtio_init, NULL, &spi_mcux_data_##n,                 \
				  &spi_mcux_config_##n, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,     \
				  &spi_mcux_rtio_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_MCUX_LPSPI_RTIO_INIT)
