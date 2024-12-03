/*
 * Copyright 2018, 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpspi

#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_mcux_lpspi, CONFIG_SPI_LOG_LEVEL);

#ifdef CONFIG_SPI_RTIO
#include <zephyr/drivers/spi/rtio.h>
#endif

#include "spi_context.h"

#if CONFIG_NXP_LP_FLEXCOMM
#include <zephyr/drivers/mfd/nxp_lp_flexcomm.h>
#endif

#include <fsl_lpspi.h>

/* If any hardware revisions change this, make it into a DT property.
 * DONT'T make #ifdefs here by platform.
 */
#define CHIP_SELECT_COUNT 4
#define MAX_DATA_WIDTH    4096

/* Required by DEVICE_MMIO_NAMED_* macros */
#define DEV_CFG(_dev)  ((const struct spi_mcux_config *)(_dev)->config)
#define DEV_DATA(_dev) ((struct spi_mcux_data *)(_dev)->data)

/* Argument to MCUX SDK IRQ handler */
#define LPSPI_IRQ_HANDLE_ARG COND_CODE_1(CONFIG_NXP_LP_FLEXCOMM, (LPSPI_GetInstance(base)), (base))

/* flag for SDK API for master transfers */
#define LPSPI_MASTER_XFER_CFG_FLAGS(slave)                                                         \
	kLPSPI_MasterPcsContinuous | (slave << LPSPI_MASTER_PCS_SHIFT)

#ifdef CONFIG_SPI_MCUX_LPSPI_DMA
#include <zephyr/drivers/dma.h>

/* These flags are arbitrary */
#define LPSPI_DMA_ERROR_FLAG   BIT(0)
#define LPSPI_DMA_RX_DONE_FLAG BIT(1)
#define LPSPI_DMA_TX_DONE_FLAG BIT(2)
#define LPSPI_DMA_DONE_FLAG    (LPSPI_DMA_RX_DONE_FLAG | LPSPI_DMA_TX_DONE_FLAG)

struct spi_dma_stream {
	const struct device *dma_dev;
	uint32_t channel; /* stores the channel for dma */
	struct dma_config dma_cfg;
	struct dma_block_config dma_blk_cfg;
};
#endif /* CONFIG_SPI_MCUX_LPSPI_DMA */

struct spi_mcux_config {
	DEVICE_MMIO_NAMED_ROM(reg_base);
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	void (*irq_config_func)(const struct device *dev);
	uint32_t pcs_sck_delay;
	uint32_t sck_pcs_delay;
	uint32_t transfer_delay;
	const struct pinctrl_dev_config *pincfg;
	lpspi_pin_config_t data_pin_config;
};

struct spi_mcux_data {
	DEVICE_MMIO_NAMED_RAM(reg_base);
	const struct device *dev;
	lpspi_master_handle_t handle;
	struct spi_context ctx;
	size_t transfer_len;
#ifdef CONFIG_SPI_RTIO
	struct spi_rtio *rtio_ctx;
#endif
#ifdef CONFIG_SPI_MCUX_LPSPI_DMA
	volatile uint32_t status_flags;
	struct spi_dma_stream dma_rx;
	struct spi_dma_stream dma_tx;
	/* dummy value used for transferring NOP when tx buf is null */
	uint32_t dummy_buffer;
#endif
};

static int spi_mcux_transfer_next_packet(const struct device *dev);
#ifdef CONFIG_SPI_RTIO
static void spi_mcux_iodev_complete(const struct device *dev, int status);
#endif

static void spi_mcux_isr(const struct device *dev)
{
	struct spi_mcux_data *data = dev->data;
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);

	LPSPI_MasterTransferHandleIRQ(LPSPI_IRQ_HANDLE_ARG, &data->handle);
}

static void spi_mcux_master_callback(LPSPI_Type *base, lpspi_master_handle_t *handle,
				     status_t status, void *userData)
{
	struct spi_mcux_data *data = userData;

#ifdef CONFIG_SPI_RTIO
	struct spi_rtio *rtio_ctx = data->rtio_ctx;

	if (rtio_ctx->txn_head != NULL) {
		spi_mcux_iodev_complete(data->dev, status);
		return;
	}
#endif
	spi_context_update_tx(&data->ctx, 1, data->transfer_len);
	spi_context_update_rx(&data->ctx, 1, data->transfer_len);

	spi_mcux_transfer_next_packet(data->dev);
}

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

static int spi_mcux_configure(const struct device *dev, const struct spi_config *spi_cfg)
{
	const struct spi_mcux_config *config = dev->config;
	struct spi_mcux_data *data = dev->data;
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	uint32_t word_size = SPI_WORD_SIZE_GET(spi_cfg->operation);
	lpspi_master_config_t master_config;
	uint32_t clock_freq;

	if (spi_cfg->operation & SPI_HALF_DUPLEX) {
		LOG_ERR("Half-duplex not supported");
		return -ENOTSUP;
	}

	if (spi_cfg->slave > CHIP_SELECT_COUNT) {
		LOG_ERR("Slave %d is greater than %d", spi_cfg->slave, CHIP_SELECT_COUNT);
		return -EINVAL;
	}

	if (word_size > MAX_DATA_WIDTH) {
		LOG_ERR("Word size %d is greater than %d", word_size, MAX_DATA_WIDTH);
		return -EINVAL;
	}

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys, &clock_freq)) {
		return -EINVAL;
	}

	if (data->ctx.config != NULL) {
		/* Setting the baud rate in LPSPI_MasterInit requires module to be disabled. Only
		 * disable if already configured, otherwise the clock is not enabled and the
		 * CR register cannot be written.
		 */
		LPSPI_Enable(base, false);
		while ((base->CR & LPSPI_CR_MEN_MASK) != 0U) {
			/* Wait until LPSPI is disabled. Datasheet:
			 * After writing 0, MEN (Module Enable) remains set until the LPSPI has
			 * completed the current transfer and is idle.
			 */
		}
	}

	if (IS_ENABLED(CONFIG_DEBUG)) {
		base->CR |= LPSPI_CR_DBGEN_MASK;
	}

	data->ctx.config = spi_cfg;

	LPSPI_MasterGetDefaultConfig(&master_config);

	master_config.bitsPerFrame = word_size;
	master_config.cpol = (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPOL)
				     ? kLPSPI_ClockPolarityActiveLow
				     : kLPSPI_ClockPolarityActiveHigh;
	master_config.cpha = (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPHA)
				     ? kLPSPI_ClockPhaseSecondEdge
				     : kLPSPI_ClockPhaseFirstEdge;
	master_config.direction =
		(spi_cfg->operation & SPI_TRANSFER_LSB) ? kLPSPI_LsbFirst : kLPSPI_MsbFirst;
	master_config.baudRate = spi_cfg->frequency;
	master_config.pcsToSckDelayInNanoSec = config->pcs_sck_delay;
	master_config.lastSckToPcsDelayInNanoSec = config->sck_pcs_delay;
	master_config.betweenTransferDelayInNanoSec = config->transfer_delay;
	master_config.pinCfg = config->data_pin_config;

	LPSPI_MasterInit(base, &master_config, clock_freq);
	LPSPI_MasterTransferCreateHandle(base, &data->handle, spi_mcux_master_callback, data);
	LPSPI_SetDummyData(base, 0);

	return 0;
}

#ifdef CONFIG_SPI_MCUX_LPSPI_DMA
static bool lpspi_inst_has_dma(const struct spi_mcux_data *data)
{
	return (data->dma_tx.dma_dev && data->dma_rx.dma_dev);
}

/* This function is executed in the interrupt context */
static void spi_mcux_dma_callback(const struct device *dev, void *arg, uint32_t channel, int status)
{
	/* arg directly holds the spi device */
	const struct device *spi_dev = arg;
	struct spi_mcux_data *data = (struct spi_mcux_data *)spi_dev->data;
	char debug_char;

	if (status < 0) {
		goto error;
	}

	/* identify the origin of this callback */
	if (channel == data->dma_tx.channel) {
		/* this part of the transfer ends */
		data->status_flags |= LPSPI_DMA_TX_DONE_FLAG;
		debug_char = 'T';
	} else if (channel == data->dma_rx.channel) {
		/* this part of the transfer ends */
		data->status_flags |= LPSPI_DMA_RX_DONE_FLAG;
		debug_char = 'R';
	} else {
		goto error;
	}

	LOG_DBG("DMA %cX Block Complete", debug_char);

#if CONFIG_SPI_ASYNC
	if (data->ctx.asynchronous && (data->status_flags & LPSPI_DMA_DONE_FLAG)) {
		/* Load dma blocks of equal length */
		size_t dma_size = spi_context_max_continuous_chunk(data->ctx);

		if (dma_size != 0) {
			return;
		}

		spi_context_update_tx(&data->ctx, 1, dma_size);
		spi_context_update_rx(&data->ctx, 1, dma_size);
	}
#endif

	goto done;
error:
	LOG_ERR("DMA callback error with channel %d.", channel);
	data->status_flags |= LPSPI_DMA_ERROR_FLAG;
done:
	spi_context_complete(&data->ctx, spi_dev, 0);
}

static struct dma_block_config *spi_mcux_dma_common_load(struct spi_dma_stream *stream,
							 const struct device *dev,
							 const uint8_t *buf, size_t len)
{
	struct spi_mcux_data *data = dev->data;
	struct dma_block_config *blk_cfg = &stream->dma_blk_cfg;

	/* prepare the block for this TX DMA channel */
	memset(blk_cfg, 0, sizeof(struct dma_block_config));

	blk_cfg->block_size = len;

	if (buf == NULL) {
		blk_cfg->source_address = (uint32_t)&data->dummy_buffer;
		blk_cfg->dest_address = (uint32_t)&data->dummy_buffer;
		/* pretend it is peripheral xfer so DMA just xfer to dummy buf */
		stream->dma_cfg.channel_direction = PERIPHERAL_TO_PERIPHERAL;
	} else {
		blk_cfg->source_address = (uint32_t)buf;
		blk_cfg->dest_address = (uint32_t)buf;
	}

	/* Transfer 1 byte each DMA loop */
	stream->dma_cfg.source_burst_length = 1;
	stream->dma_cfg.user_data = (void *)dev;
	stream->dma_cfg.head_block = blk_cfg;

	return blk_cfg;
}

static int spi_mcux_dma_tx_load(const struct device *dev, const uint8_t *buf, size_t len)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	struct spi_mcux_data *data = dev->data;
	/* remember active TX DMA channel (used in callback) */
	struct spi_dma_stream *stream = &data->dma_tx;
	struct dma_block_config *blk_cfg = spi_mcux_dma_common_load(stream, dev, buf, len);

	if (buf != NULL) {
		/* tx direction has memory as source and periph as dest. */
		stream->dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
	}

	/* Dest is LPSPI tx fifo */
	blk_cfg->dest_address = LPSPI_GetTxRegisterAddress(base);

	/* give the client dev as arg, as the callback comes from the dma */
	/* pass our client origin to the dma: data->dma_tx.dma_channel */
	return dma_config(stream->dma_dev, stream->channel, &stream->dma_cfg);
}

static int spi_mcux_dma_rx_load(const struct device *dev, uint8_t *buf, size_t len)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	struct spi_mcux_data *data = dev->data;
	/* retrieve active RX DMA channel (used in callback) */
	struct spi_dma_stream *stream = &data->dma_rx;
	struct dma_block_config *blk_cfg = spi_mcux_dma_common_load(stream, dev, buf, len);

	if (buf != NULL) {
		/* rx direction has periph as source and mem as dest. */
		stream->dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
	}

	/* Source is LPSPI rx fifo */
	blk_cfg->source_address = LPSPI_GetRxRegisterAddress(base);

	/* pass our client origin to the dma: data->dma_rx.channel */
	return dma_config(stream->dma_dev, stream->channel, &stream->dma_cfg);
}

static int wait_dma_rx_tx_done(const struct device *dev)
{
	struct spi_mcux_data *data = dev->data;
	int ret;

	do {
		ret = spi_context_wait_for_completion(&data->ctx);
		if (ret) {
			LOG_DBG("Timed out waiting for SPI context to complete");
			return ret;
		} else if (data->status_flags & LPSPI_DMA_ERROR_FLAG) {
			return -EIO;
		}
	} while (!((data->status_flags & LPSPI_DMA_DONE_FLAG) == LPSPI_DMA_DONE_FLAG));

	LOG_DBG("DMA block completed");
	return 0;
}

static inline int spi_mcux_dma_rxtx_load(const struct device *dev, size_t *dma_size)
{
	struct spi_mcux_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int ret = 0;

	/* Clear status flags */
	data->status_flags = 0U;

	/* Load dma blocks of equal length */
	*dma_size = spi_context_max_continuous_chunk(ctx);

	ret = spi_mcux_dma_tx_load(dev, ctx->tx_buf, *dma_size);
	if (ret != 0) {
		return ret;
	}

	ret = spi_mcux_dma_rx_load(dev, ctx->rx_buf, *dma_size);
	if (ret != 0) {
		return ret;
	}

	/* Start DMA */
	ret = dma_start(data->dma_tx.dma_dev, data->dma_tx.channel);
	if (ret != 0) {
		return ret;
	}

	ret = dma_start(data->dma_rx.dma_dev, data->dma_rx.channel);
	return ret;
}

#ifdef CONFIG_SPI_ASYNC
static int transceive_dma_async(const struct device *dev, spi_callback_t cb, void *userdata)
{
	struct spi_mcux_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	size_t dma_size;
	int ret;

	ctx->asynchronous = true;
	ctx->callback = cb;
	ctx->callback_data = userdata;

	ret = spi_mcux_dma_rxtx_load(dev, &dma_size);
	if (ret) {
		return ret;
	}

	/* Enable DMA Requests */
	LPSPI_EnableDMA(base, kLPSPI_TxDmaEnable | kLPSPI_RxDmaEnable);

	return 0;
}
#else
#define transceive_dma_async(...) 0
#endif /* CONFIG_SPI_ASYNC */

static int transceive_dma_sync(const struct device *dev)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	struct spi_mcux_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	size_t dma_size;
	int ret;

	spi_context_cs_control(ctx, true);

	/* Send each spi buf via DMA, updating context as DMA completes */
	while (ctx->rx_len > 0 || ctx->tx_len > 0) {
		/* Load dma block */
		ret = spi_mcux_dma_rxtx_load(dev, &dma_size);
		if (ret) {
			return ret;
		}

#ifdef CONFIG_SOC_SERIES_MCXN
		while (!(LPSPI_GetStatusFlags(base) & kLPSPI_TxDataRequestFlag)) {
			/* wait until previous tx finished */
		}
#endif

		/* Enable DMA Requests */
		LPSPI_EnableDMA(base, kLPSPI_TxDmaEnable | kLPSPI_RxDmaEnable);

		/* Wait for DMA to finish */
		ret = wait_dma_rx_tx_done(dev);
		if (ret) {
			return ret;
		}

#ifndef CONFIG_SOC_SERIES_MCXN
		while ((LPSPI_GetStatusFlags(base) & kLPSPI_ModuleBusyFlag)) {
			/* wait until module is idle */
		}
#endif

		/* Disable DMA */
		LPSPI_DisableDMA(base, kLPSPI_TxDmaEnable | kLPSPI_RxDmaEnable);

		/* Update SPI contexts with amount of data we just sent */
		spi_context_update_tx(ctx, 1, dma_size);
		spi_context_update_rx(ctx, 1, dma_size);
	}

	spi_context_cs_control(ctx, false);

	base->TCR = 0;

	return 0;
}

static int transceive_dma(const struct device *dev, const struct spi_config *spi_cfg,
			  const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs,
			  bool asynchronous, spi_callback_t cb, void *userdata)
{
	struct spi_mcux_data *data = dev->data;
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	int ret;

	if (!asynchronous) {
		spi_context_lock(&data->ctx, asynchronous, cb, userdata, spi_cfg);
	}

	ret = spi_mcux_configure(dev, spi_cfg);
	if (ret && !asynchronous) {
		goto out;
	} else if (ret) {
		return ret;
	}

#ifdef CONFIG_SOC_SERIES_MCXN
	base->TCR |= LPSPI_TCR_CONT_MASK;
#endif

	/* DMA is fast enough watermarks are not required */
	LPSPI_SetFifoWatermarks(base, 0U, 0U);

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

	if (asynchronous) {
		ret = transceive_dma_async(dev, cb, userdata);
	} else {
		ret = transceive_dma_sync(dev);
	}

out:
	spi_context_release(&data->ctx, ret);
	return ret;
}
#else
#define lpspi_inst_has_dma(arg) arg != arg
#define transceive_dma(...)     0
#endif /* CONFIG_SPI_MCUX_LPSPI_DMA */

#ifdef CONFIG_SPI_RTIO
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

static inline int transceive_rtio(const struct device *dev, const struct spi_config *spi_cfg,
				  const struct spi_buf_set *tx_bufs,
				  const struct spi_buf_set *rx_bufs)
{
	struct spi_mcux_data *data = dev->data;
	struct spi_rtio *rtio_ctx = data->rtio_ctx;
	int ret;

	spi_context_lock(&data->ctx, false, NULL, NULL, spi_cfg);

	ret = spi_rtio_transceive(rtio_ctx, spi_cfg, tx_bufs, rx_bufs);

	spi_context_release(&data->ctx, ret);

	return ret;
}
#else
#define transceive_rtio(...) 0
#endif /* CONFIG_SPI_RTIO */

static int transceive(const struct device *dev, const struct spi_config *spi_cfg,
		      const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs,
		      bool asynchronous, spi_callback_t cb, void *userdata)
{
	struct spi_mcux_data *data = dev->data;
	int ret;

	spi_context_lock(&data->ctx, asynchronous, cb, userdata, spi_cfg);

	ret = spi_mcux_configure(dev, spi_cfg);
	if (ret) {
		goto out;
	}

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

static int spi_mcux_transceive(const struct device *dev, const struct spi_config *spi_cfg,
			       const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs,
			       spi_callback_t cb, void *userdata, bool async)
{
	struct spi_mcux_data *data = dev->data;

	if (lpspi_inst_has_dma(data)) {
		return transceive_dma(dev, spi_cfg, tx_bufs, rx_bufs, async, cb, userdata);
	}

	if (IS_ENABLED(CONFIG_SPI_RTIO)) {
		return transceive_rtio(dev, spi_cfg, tx_bufs, rx_bufs);
	}

	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, async, cb, userdata);
}

static int spi_mcux_transceive_sync(const struct device *dev, const struct spi_config *spi_cfg,
				    const struct spi_buf_set *tx_bufs,
				    const struct spi_buf_set *rx_bufs)
{
	return spi_mcux_transceive(dev, spi_cfg, tx_bufs, rx_bufs, NULL, NULL, false);
}

#ifdef CONFIG_SPI_ASYNC
static int spi_mcux_transceive_async(const struct device *dev, const struct spi_config *spi_cfg,
				     const struct spi_buf_set *tx_bufs,
				     const struct spi_buf_set *rx_bufs, spi_callback_t cb,
				     void *userdata)
{
	return spi_mcux_transceive(dev, spi_cfg, tx_bufs, rx_bufs, cb, userdata, true);
}
#endif /* CONFIG_SPI_ASYNC */

static int spi_mcux_release(const struct device *dev, const struct spi_config *spi_cfg)
{
	struct spi_mcux_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(spi, spi_mcux_driver_api) = {
	.transceive = spi_mcux_transceive_sync,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_mcux_transceive_async,
#endif
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_mcux_iodev_submit,
#endif
	.release = spi_mcux_release,
};

#if defined(CONFIG_SPI_MCUX_LPSPI_DMA)
static int lpspi_dma_dev_ready(const struct device *dma_dev)
{
	if (!device_is_ready(dma_dev)) {
		LOG_ERR("%s device is not ready", dma_dev->name);
		return -ENODEV;
	}

	return 0;
}

static int lpspi_dma_devs_ready(struct spi_mcux_data *data)
{
	return lpspi_dma_dev_ready(data->dma_tx.dma_dev) |
	       lpspi_dma_dev_ready(data->dma_rx.dma_dev);
}
#else
#define lpspi_dma_devs_ready(...) 0
#endif /* CONFIG_SPI_MCUX_LPSPI_DMA */

static int spi_mcux_init(const struct device *dev)
{
	const struct spi_mcux_config *config = dev->config;
	struct spi_mcux_data *data = dev->data;
	int err = 0;

	DEVICE_MMIO_NAMED_MAP(dev, reg_base, K_MEM_CACHE_NONE | K_MEM_DIRECT_MAP);

	data->dev = dev;

	if (IS_ENABLED(CONFIG_SPI_MCUX_LPSPI_DMA) && lpspi_inst_has_dma(data)) {
		err = lpspi_dma_devs_ready(data);
	}
	if (err < 0) {
		return err;
	}

	err = spi_context_cs_configure_all(&data->ctx);
	if (err < 0) {
		return err;
	}

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}

	config->irq_config_func(dev);

#ifdef CONFIG_SPI_RTIO
	spi_rtio_init(data->rtio_ctx, dev);
#endif
	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#define SPI_MCUX_RTIO_DEFINE(n)                                                                    \
	SPI_RTIO_DEFINE(spi_mcux_rtio_##n, CONFIG_SPI_MCUX_RTIO_SQ_SIZE,                           \
			CONFIG_SPI_MCUX_RTIO_SQ_SIZE)

#ifdef CONFIG_SPI_MCUX_LPSPI_DMA
#define SPI_DMA_CHANNELS(n)                                                                        \
	IF_ENABLED(                                                                                \
		DT_INST_DMAS_HAS_NAME(n, tx),                                                      \
		(.dma_tx = {.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(n, tx)),            \
			    .channel = DT_INST_DMAS_CELL_BY_NAME(n, tx, mux),                      \
			    .dma_cfg = {.channel_direction = MEMORY_TO_PERIPHERAL,                 \
					.dma_callback = spi_mcux_dma_callback,                     \
					.source_data_size = 1,                                     \
					.dest_data_size = 1,                                       \
					.block_count = 1,                                          \
					.dma_slot = DT_INST_DMAS_CELL_BY_NAME(n, tx, source)}},))  \
	IF_ENABLED(                                                                                \
		DT_INST_DMAS_HAS_NAME(n, rx),                                                      \
		(.dma_rx = {.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(n, rx)),            \
			    .channel = DT_INST_DMAS_CELL_BY_NAME(n, rx, mux),                      \
			    .dma_cfg = {.channel_direction = PERIPHERAL_TO_MEMORY,                 \
					.dma_callback = spi_mcux_dma_callback,                     \
					.source_data_size = 1,                                     \
					.dest_data_size = 1,                                       \
					.block_count = 1,                                          \
					.dma_slot = DT_INST_DMAS_CELL_BY_NAME(n, rx, source)}},))
#else
#define SPI_DMA_CHANNELS(n)
#endif /* CONFIG_SPI_MCUX_LPSPI_DMA */

#if defined(CONFIG_NXP_LP_FLEXCOMM)
#define SPI_MCUX_LPSPI_IRQ_FUNC(n)                                                                 \
	nxp_lp_flexcomm_setirqhandler(DEVICE_DT_GET(DT_INST_PARENT(n)), DEVICE_DT_INST_GET(n),     \
				      LP_FLEXCOMM_PERIPH_LPSPI, spi_mcux_isr);
#else
#define SPI_MCUX_LPSPI_IRQ_FUNC(n)                                                                 \
	IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), spi_mcux_isr,                       \
		    DEVICE_DT_INST_GET(n), 0);                                                     \
	irq_enable(DT_INST_IRQN(n));
#endif

#define SPI_MCUX_LPSPI_INIT(n)                                                                     \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	COND_CODE_1(CONFIG_SPI_RTIO, (SPI_MCUX_RTIO_DEFINE(n)), ());                               \
                                                                                                   \
	static void spi_mcux_config_func_##n(const struct device *dev)                             \
	{                                                                                          \
		SPI_MCUX_LPSPI_IRQ_FUNC(n)                                                         \
	}                                                                                          \
                                                                                                   \
	static const struct spi_mcux_config spi_mcux_config_##n = {                                \
		DEVICE_MMIO_NAMED_ROM_INIT(reg_base, DT_DRV_INST(n)),                              \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),              \
		.irq_config_func = spi_mcux_config_func_##n,                                       \
		.pcs_sck_delay = UTIL_AND(DT_INST_NODE_HAS_PROP(n, pcs_sck_delay),                 \
					  DT_INST_PROP(n, pcs_sck_delay)),                         \
		.sck_pcs_delay = UTIL_AND(DT_INST_NODE_HAS_PROP(n, sck_pcs_delay),                 \
					  DT_INST_PROP(n, sck_pcs_delay)),                         \
		.transfer_delay = UTIL_AND(DT_INST_NODE_HAS_PROP(n, transfer_delay),               \
					   DT_INST_PROP(n, transfer_delay)),                       \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                       \
		.data_pin_config = DT_INST_ENUM_IDX(n, data_pin_config),                           \
	};                                                                                         \
                                                                                                   \
	static struct spi_mcux_data spi_mcux_data_##n = {                                          \
		SPI_CONTEXT_INIT_LOCK(spi_mcux_data_##n, ctx),                                     \
		SPI_CONTEXT_INIT_SYNC(spi_mcux_data_##n, ctx),                                     \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx) SPI_DMA_CHANNELS(n)           \
			IF_ENABLED(CONFIG_SPI_RTIO, (.rtio_ctx = &spi_mcux_rtio_##n,))             \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, spi_mcux_init, NULL, &spi_mcux_data_##n, &spi_mcux_config_##n,    \
			      POST_KERNEL, CONFIG_SPI_INIT_PRIORITY, &spi_mcux_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_MCUX_LPSPI_INIT)
