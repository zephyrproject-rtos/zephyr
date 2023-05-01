/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright (c) 2017, 2020-2021, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT	nxp_kinetis_dspi

#include <errno.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/clock_control.h>
#include <fsl_dspi.h>
#include <zephyr/drivers/pinctrl.h>
#ifdef CONFIG_DSPI_MCUX_EDMA
#include <zephyr/drivers/dma.h>
#include <fsl_edma.h>
#endif

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(spi_mcux_dspi);

#include "spi_context.h"

#ifdef CONFIG_DSPI_MCUX_EDMA

struct spi_edma_config {
	const struct device *dma_dev;
	int32_t state;
	uint32_t dma_channel;
	void (*irq_call_back)(void);
	struct dma_config dma_cfg;
};
#endif

struct spi_mcux_config {
	SPI_Type *base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	void (*irq_config_func)(const struct device *dev);
	uint32_t pcs_sck_delay;
	uint32_t sck_pcs_delay;
	uint32_t transfer_delay;
	uint32_t which_ctar;
	uint32_t samplePoint;
	bool enable_continuous_sck;
	bool enable_rxfifo_overwrite;
	bool enable_modified_timing_format;
	bool is_dma_chn_shared;
	const struct pinctrl_dev_config *pincfg;
};

struct spi_mcux_data {
	const struct device *dev;
	dspi_master_handle_t handle;
	struct spi_context ctx;
	size_t transfer_len;
#ifdef CONFIG_DSPI_MCUX_EDMA
	struct dma_block_config tx_dma_block;
	struct dma_block_config tx_dma_block_end;
	struct dma_block_config rx_dma_block;
	struct spi_edma_config rx_dma_config;
	struct spi_edma_config tx_dma_config;
	int frame_size;
	int tx_transfer_count;
	int rx_transfer_count;
	uint32_t which_pcs;
	struct spi_buf *inner_tx_buffer;
	struct spi_buf *inner_rx_buffer;
#endif
};

#ifdef CONFIG_DSPI_MCUX_EDMA
static int get_size_byte_by_frame_size(int len, int frame_size)
{
	if (frame_size == 8) {
		return (len * 4);
	} else { /* frame_size == 16*/
		return (len * 2);
	}
}
#endif

static int spi_mcux_transfer_next_packet(const struct device *dev)
{
	const struct spi_mcux_config *config = dev->config;
	struct spi_mcux_data *data = dev->data;
	SPI_Type *base = config->base;
	struct spi_context *ctx = &data->ctx;
	dspi_transfer_t transfer;
	status_t status;

	if ((ctx->tx_len == 0) && (ctx->rx_len == 0)) {
		/* nothing left to rx or tx, we're done! */
		LOG_DBG("spi transceive done");
		spi_context_cs_control(&data->ctx, false);
		spi_context_complete(&data->ctx, dev, 0);
		return 0;
	}

#ifdef CONFIG_DSPI_MCUX_EDMA

	if (!config->is_dma_chn_shared) {
		/* start dma directly in not shared mode */
		if (ctx->tx_len != 0) {
			int ret = 0;

			LOG_DBG("Starting DMA Ch%u",
				data->tx_dma_config.dma_channel);
			ret = dma_start(data->tx_dma_config.dma_dev,
					data->tx_dma_config.dma_channel);
			if (ret < 0) {
				LOG_ERR("Failed to start DMA Ch%d (%d)",
					data->tx_dma_config.dma_channel, ret);
				return ret;
			}
		}

		if (ctx->rx_len != 0) {
			int ret = 0;

			LOG_DBG("Starting DMA Ch%u",
				data->rx_dma_config.dma_channel);
			ret = dma_start(data->rx_dma_config.dma_dev,
					data->rx_dma_config.dma_channel);
			if (ret < 0) {
				LOG_ERR("Failed to start DMA Ch%d (%d)",
					data->rx_dma_config.dma_channel, ret);
				return ret;
			}
		}
	}

	DSPI_EnableDMA(base, (uint32_t)kDSPI_RxDmaEnable |
					      (uint32_t)kDSPI_TxDmaEnable);
	DSPI_StartTransfer(base);

	if (config->is_dma_chn_shared) {
		/* in master mode start tx */
		dma_start(data->tx_dma_config.dma_dev, data->tx_dma_config.dma_channel);
		/* TBD kDSPI_TxFifoFillRequestFlag */
		DSPI_EnableInterrupts(base,
				      (uint32_t)kDSPI_RxFifoDrainRequestFlag);
		LOG_DBG("trigger tx to start master");
	}

	return 0;
#endif

	transfer.configFlags = kDSPI_MasterCtar0 | kDSPI_MasterPcsContinuous |
			       (ctx->config->slave << DSPI_MASTER_PCS_SHIFT);

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
		transfer.configFlags |= kDSPI_MasterActiveAfterTransfer;
	} else {
		/* Break up the rx into multiple transfers so we don't have to
		 * tx from a longer intermediate buffer. Leave chip select
		 * active between transfers.
		 */
		transfer.txData = (uint8_t *) ctx->tx_buf;
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
		LOG_ERR("Transfer could not start");
	}

	return status == kStatus_Success ? 0 :
	       status == kDSPI_Busy ? -EBUSY : -EINVAL;
}

static void spi_mcux_isr(const struct device *dev)
{
	const struct spi_mcux_config *config = dev->config;
	struct spi_mcux_data *data = dev->data;
	SPI_Type *base = config->base;

#ifdef CONFIG_DSPI_MCUX_EDMA
	LOG_DBG("isr is called");

	if (0U != (DSPI_GetStatusFlags(base) &
		   (uint32_t)kDSPI_RxFifoDrainRequestFlag)) {
		/* start rx */
		dma_start(data->rx_dma_config.dma_dev, data->rx_dma_config.dma_channel);
	}
#else
	DSPI_MasterTransferHandleIRQ(base, &data->handle);
#endif
}

#ifdef CONFIG_DSPI_MCUX_EDMA

static void mcux_init_inner_buffer_with_cmd(const struct device *dev,
					    uint16_t dummy)
{
	const struct spi_mcux_config *config = dev->config;
	struct spi_mcux_data *data = dev->data;
	dspi_command_data_config_t commandStruct;
	uint32_t *pbuf = data->inner_tx_buffer->buf;
	uint32_t command;
	int i = 0;

	commandStruct.whichPcs = data->which_pcs;

	commandStruct.isEndOfQueue = false;
	commandStruct.clearTransferCount = false;
	commandStruct.whichCtar = config->which_ctar;
	commandStruct.isPcsContinuous = config->enable_continuous_sck;
	command = DSPI_MasterGetFormattedCommand(&(commandStruct));
	for (i = 0; i < data->inner_tx_buffer->len / 4; i++) {
		*pbuf = command | dummy;
		pbuf++;
	}
}

/**
 * @brief update the tx data to internal buffer with command embedded,
 * if no tx data, use dummy value.
 * tx data frame size shall not bigger than 16 bits
 * the overall transfer data in one batch shall not larger than FIFO size
 */
static int mcux_spi_context_data_update(const struct device *dev)
{
	struct spi_mcux_data *data = dev->data;
	uint32_t frame_size_bit = data->frame_size;
	struct spi_context *ctx = (struct spi_context *)&data->ctx;
	uint32_t *pcdata = data->inner_tx_buffer->buf;

	if (frame_size_bit > FSL_FEATURE_DSPI_MAX_DATA_WIDTH) {
		/* TODO need set to continues PCS to have frame size larger than 16 */
		LOG_ERR("frame size is larger than 16");
		return -EINVAL;
	}

#ifdef CONFIG_MCUX_DSPI_EDMA_SHUFFLE_DATA
	/* only used when use inner buffer to translate tx format */
	if (CONFIG_MCUX_DSPI_BUFFER_SIZE * 4 <
	    get_size_byte_by_frame_size(ctx->current_tx->len, frame_size_bit)) {
		/* inner buffer can not hold all transferred data */
		LOG_ERR("inner buffer is too small to hold all data esp %d, act %d",
			ctx->current_tx->len * 8 / frame_size_bit,
			(CONFIG_MCUX_DSPI_BUFFER_SIZE * 4 / frame_size_bit));
		return -EINVAL;
	}

	if (frame_size_bit == 8) {
		int i = 0;
		uint8_t *pdata = (uint8_t *)ctx->tx_buf;

		if (pdata) {
			do {
				uint16_t temp_data = 0;

				temp_data = *pdata;
				pdata++;
				*pcdata |= temp_data;
				pcdata++;
				i++;
			} while (i < ctx->current_tx->len &&
				 i < data->inner_tx_buffer->len);
		}
		/* indicate it is the last data */
		if (i == ctx->current_tx->len) {
			--pcdata;
			*pcdata |= SPI_PUSHR_EOQ(1) | SPI_PUSHR_CTCNT(1);
			LOG_DBG("last pcdata is %x", *pcdata);
		}
	} else if (frame_size_bit == 16) {
		int i = 0;
		uint16_t *pdata = (uint16_t *)ctx->tx_buf;

		if (pdata) {
			do {
				*pcdata |= *pdata;
				LOG_DBG("pcdata %d is %x", i / 2, *pcdata);
				pdata++;
				pcdata++;
				i += 2;
			} while (i < ctx->current_tx->len &&
				 i < data->inner_tx_buffer->len);
		}
		if (i == ctx->current_tx->len) {
			/* indicate it is the last data */
			--pcdata;
			*pcdata |= SPI_PUSHR_EOQ(1);
			LOG_DBG("last pcdata is %x", *pcdata);
		}
	} else {
		/* TODO for other size */
		LOG_ERR("DMA mode only support 8/16 bits frame size");
		return -EINVAL;
	}

#endif /* CONFIG_MCUX_DSPI_EDMA_SHUFFLE_DATA */

	return 0;
}

static int update_tx_dma(const struct device *dev)
{
	uint32_t tx_size = 0;
	uint8_t *tx_buf;
	struct spi_mcux_data *data = dev->data;
	const struct spi_mcux_config *config = dev->config;
	SPI_Type *base = config->base;
	uint32_t frame_size = data->frame_size;
	bool rx_only = false;

	DSPI_DisableDMA(base, (uint32_t)kDSPI_TxDmaEnable);
	if (data->ctx.tx_len == 0) {
		LOG_DBG("empty data no need to setup DMA");
		return 0;
	}

	if (data->ctx.current_tx && data->ctx.current_tx->len > 0 &&
	    data->ctx.current_tx->buf != NULL) {
#ifdef CONFIG_MCUX_DSPI_EDMA_SHUFFLE_DATA
		tx_size = get_size_byte_by_frame_size(data->transfer_len,
						      frame_size);
		tx_buf = data->inner_tx_buffer->buf;
#else
		/* expect the buffer is pre-set */
		tx_size = get_size_byte_by_frame_size(data->ctx.current_tx->len,
						      frame_size);
		LOG_DBG("tx size is %d", tx_size);
		tx_buf = data->ctx.current_tx->buf;
#endif
	} else {
		tx_buf = data->inner_tx_buffer->buf;
		tx_size = get_size_byte_by_frame_size(data->transfer_len,
						      frame_size);
		rx_only = true;
		LOG_DBG("rx only 0x%x, size %d", (uint32_t)tx_buf, tx_size);
	}

	data->tx_dma_block.source_address = (uint32_t)tx_buf;
	data->tx_dma_block.dest_address =
		DSPI_MasterGetTxRegisterAddress(base);
	data->tx_dma_block.next_block = NULL;
	if (config->is_dma_chn_shared) {
		/* transfer FIFO size data */
		data->tx_dma_block.block_size = 4;
	} else {
		data->tx_dma_block.block_size = tx_size;
	}

	data->tx_dma_config.dma_cfg.user_data = (void *) dev;
	dma_config(data->tx_dma_config.dma_dev, data->tx_dma_config.dma_channel,
		   (struct dma_config *)&data->tx_dma_config.dma_cfg);

	return 0;
}

static int update_rx_dma(const struct device *dev)
{
	uint32_t rx_size = 0;
	uint8_t *rx_buf;
	struct spi_mcux_data *data = dev->data;
	const struct spi_mcux_config *config = dev->config;
	SPI_Type *base = config->base;
	uint32_t frame_size_byte = (data->frame_size >> 3);
	bool tx_only = false;

	DSPI_DisableDMA(base, (uint32_t)kDSPI_RxDmaEnable);
	if (data->ctx.rx_len == 0) {
		LOG_DBG("empty data no need to setup DMA");
		return 0;
	}

	if (data->ctx.current_rx) {
		rx_size = data->transfer_len;
		if (data->ctx.rx_buf != NULL) {
			rx_buf = data->ctx.rx_buf;
		} else {
			rx_buf = data->inner_rx_buffer->buf;
		}
	} else {
		/* tx only */
		rx_buf = data->inner_rx_buffer->buf;
		rx_size = data->transfer_len;
		tx_only = true;
		LOG_DBG("tx only 0x%x, size %d", (uint32_t)rx_buf, rx_size);
	}

	if (config->is_dma_chn_shared) {
		if (data->ctx.rx_len == 1) {
			/* do not link tx on last frame*/
			LOG_DBG("do not link tx/rx channel for last one");
			data->rx_dma_config.dma_cfg.source_chaining_en = 0;
			data->rx_dma_config.dma_cfg.dest_chaining_en = 0;
		} else {
			LOG_DBG("shared mux mode, link tx/rx channel");
			data->rx_dma_config.dma_cfg.source_chaining_en = 1;
			data->rx_dma_config.dma_cfg.dest_chaining_en = 1;
			data->rx_dma_config.dma_cfg.linked_channel =
				data->tx_dma_config.dma_channel;
		}

		data->rx_dma_block.dest_address = (uint32_t)rx_buf;
		data->rx_dma_block.source_address =
			DSPI_GetRxRegisterAddress(base);
		/* do once in share mode */
		data->rx_dma_block.block_size = frame_size_byte;
		data->rx_dma_config.dma_cfg.source_burst_length =
			frame_size_byte;
		data->rx_dma_config.dma_cfg.dest_burst_length = frame_size_byte;
		data->rx_dma_config.dma_cfg.source_data_size = frame_size_byte;
		data->rx_dma_config.dma_cfg.dest_data_size = frame_size_byte;

	} else {
		data->rx_dma_block.dest_address = (uint32_t)rx_buf;
		data->rx_dma_block.source_address =
			DSPI_GetRxRegisterAddress(base);
		data->rx_dma_block.block_size = rx_size;
		data->rx_dma_config.dma_cfg.source_burst_length =
			frame_size_byte;
		data->rx_dma_config.dma_cfg.dest_burst_length = frame_size_byte;
		data->rx_dma_config.dma_cfg.source_data_size = frame_size_byte;
		data->rx_dma_config.dma_cfg.dest_data_size = frame_size_byte;
	}

	data->rx_dma_config.dma_cfg.user_data = (void *) dev;
	dma_config(data->rx_dma_config.dma_dev, data->rx_dma_config.dma_channel,
		   (struct dma_config *)&data->rx_dma_config.dma_cfg);

	return 0;
}

static int configure_dma(const struct device *dev)
{
	const struct spi_mcux_config *config = dev->config;

	if (config->is_dma_chn_shared) {
		LOG_DBG("shard DMA request");
	}
	update_tx_dma(dev);
	update_rx_dma(dev);

	return 0;
}

static void dma_callback(const struct device *dma_dev, void *callback_arg,
			 uint32_t channel, int error_code)
{
	const struct device *dev = (const struct device *)callback_arg;
	const struct spi_mcux_config *config = dev->config;
	SPI_Type *base = config->base;
	struct spi_mcux_data *data = dev->data;

	LOG_DBG("=dma call back @channel %d=", channel);

	if (error_code) {
		LOG_ERR("error happened no callback process %d", error_code);
		return;
	}

	if (channel == data->tx_dma_config.dma_channel) {
		LOG_DBG("ctx.tx_len is %d", data->ctx.tx_len);
		LOG_DBG("tx count %d", data->ctx.tx_count);

		spi_context_update_tx(&data->ctx, 1, data->transfer_len);
		LOG_DBG("tx count %d", data->ctx.tx_count);
		LOG_DBG("tx buf/len %p/%zu", data->ctx.tx_buf,
			data->ctx.tx_len);
		data->tx_transfer_count++;
		/* tx done */
	} else {
		LOG_DBG("ctx.rx_len is %d", data->ctx.rx_len);
		LOG_DBG("rx count %d", data->ctx.rx_count);
		spi_context_update_rx(&data->ctx, 1, data->transfer_len);
		LOG_DBG("rx count %d", data->ctx.rx_count);
		/* setup the inner tx buffer */
		LOG_DBG("rx buf/len %p/%zu", data->ctx.rx_buf,
			data->ctx.rx_len);
		data->rx_transfer_count++;
	}

	if (data->tx_transfer_count == data->rx_transfer_count) {
		LOG_DBG("start next packet");
		DSPI_StopTransfer(base);
		DSPI_FlushFifo(base, true, true);
		DSPI_ClearStatusFlags(base,
				      (uint32_t)kDSPI_AllStatusFlag);
		mcux_init_inner_buffer_with_cmd(dev, 0);
		mcux_spi_context_data_update(dev);

		if (config->is_dma_chn_shared) {
			data->transfer_len = data->frame_size >> 3;
		} else {
			if (data->ctx.tx_len == 0) {
				data->transfer_len = data->ctx.rx_len;
			} else if (data->ctx.rx_len == 0) {
				data->transfer_len = data->ctx.tx_len;
			} else {
				data->transfer_len =
					data->ctx.tx_len > data->ctx.rx_len ?
						data->ctx.rx_len :
						data->ctx.tx_len;
			}
		}
		update_tx_dma(dev);
		update_rx_dma(dev);
		spi_mcux_transfer_next_packet(dev);
	} else if (data->ctx.rx_len == 0 && data->ctx.tx_len == 0) {
		LOG_DBG("end of transfer");
		DSPI_StopTransfer(base);
		DSPI_FlushFifo(base, true, true);
		DSPI_ClearStatusFlags(base,
				      (uint32_t)kDSPI_AllStatusFlag);
		data->transfer_len = 0;
		spi_mcux_transfer_next_packet(dev);
	}
	LOG_DBG("TX/RX DMA callback done");
}

#else

static void spi_mcux_master_transfer_callback(SPI_Type *base,
	      dspi_master_handle_t *handle, status_t status, void *userData)
{
	struct spi_mcux_data *data = userData;

	spi_context_update_tx(&data->ctx, 1, data->transfer_len);
	spi_context_update_rx(&data->ctx, 1, data->transfer_len);

	spi_mcux_transfer_next_packet(data->dev);
}

#endif /* CONFIG_DSPI_MCUX_EDMA */

static int spi_mcux_configure(const struct device *dev,
			      const struct spi_config *spi_cfg)
{
	const struct spi_mcux_config *config = dev->config;
	struct spi_mcux_data *data = dev->data;
	SPI_Type *base = config->base;
	dspi_master_config_t master_config;
	uint32_t clock_freq;
	uint32_t word_size;

	dspi_master_ctar_config_t *ctar_config = &master_config.ctarConfig;

	if (spi_context_configured(&data->ctx, spi_cfg)) {
		/* This configuration is already in use */
		return 0;
	}

	if (spi_cfg->operation & SPI_HALF_DUPLEX) {
		LOG_ERR("Half-duplex not supported");
		return -ENOTSUP;
	}

	DSPI_MasterGetDefaultConfig(&master_config);

	master_config.whichPcs = 1U << spi_cfg->slave;
	master_config.whichCtar = config->which_ctar;
	master_config.pcsActiveHighOrLow =
		(spi_cfg->operation & SPI_CS_ACTIVE_HIGH) ?
			kDSPI_PcsActiveHigh :
			kDSPI_PcsActiveLow;
	master_config.samplePoint = config->samplePoint;
	master_config.enableContinuousSCK = config->enable_continuous_sck;
	master_config.enableRxFifoOverWrite = config->enable_rxfifo_overwrite;
	master_config.enableModifiedTimingFormat =
		config->enable_modified_timing_format;

	if (spi_cfg->slave > FSL_FEATURE_DSPI_CHIP_SELECT_COUNT) {
		LOG_ERR("Slave %d is greater than %d",
			    spi_cfg->slave, FSL_FEATURE_DSPI_CHIP_SELECT_COUNT);
		return -EINVAL;
	}

	word_size = SPI_WORD_SIZE_GET(spi_cfg->operation);
	if (word_size > FSL_FEATURE_DSPI_MAX_DATA_WIDTH) {
		LOG_ERR("Word size %d is greater than %d",
			    word_size, FSL_FEATURE_DSPI_MAX_DATA_WIDTH);
		return -EINVAL;
	}

	ctar_config->bitsPerFrame = word_size;

	ctar_config->cpol =
		(SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPOL)
		? kDSPI_ClockPolarityActiveLow
		: kDSPI_ClockPolarityActiveHigh;

	ctar_config->cpha =
		(SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPHA)
		? kDSPI_ClockPhaseSecondEdge
		: kDSPI_ClockPhaseFirstEdge;

	ctar_config->direction =
		(spi_cfg->operation & SPI_TRANSFER_LSB)
		? kDSPI_LsbFirst
		: kDSPI_MsbFirst;

	ctar_config->baudRate = spi_cfg->frequency;

	ctar_config->pcsToSckDelayInNanoSec = config->pcs_sck_delay;
	ctar_config->lastSckToPcsDelayInNanoSec = config->sck_pcs_delay;
	ctar_config->betweenTransferDelayInNanoSec = config->transfer_delay;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys,
				   &clock_freq)) {
		return -EINVAL;
	}

	LOG_DBG("clock_freq is %d", clock_freq);

	DSPI_MasterInit(base, &master_config, clock_freq);

#ifdef CONFIG_DSPI_MCUX_EDMA
	DSPI_StopTransfer(base);
	DSPI_FlushFifo(base, true, true);
	DSPI_ClearStatusFlags(base, (uint32_t)kDSPI_AllStatusFlag);
	/* record frame_size setting for DMA */
	data->frame_size = word_size;
	/* keep the pcs settings */
	data->which_pcs = 1U << spi_cfg->slave;
#ifdef CONFIG_MCUX_DSPI_EDMA_SHUFFLE_DATA
	mcux_init_inner_buffer_with_cmd(dev, 0);
#endif
#else
	DSPI_MasterTransferCreateHandle(base, &data->handle,
					spi_mcux_master_transfer_callback,
					data);

	DSPI_SetDummyData(base, 0);
#endif

	data->ctx.config = spi_cfg;

	return 0;
}

static int transceive(const struct device *dev,
		      const struct spi_config *spi_cfg,
		      const struct spi_buf_set *tx_bufs,
		      const struct spi_buf_set *rx_bufs,
		      bool asynchronous,
		      spi_callback_t cb,
		      void *userdata)
{
	struct spi_mcux_data *data = dev->data;
	int ret;
#ifdef CONFIG_DSPI_MCUX_EDMA
	const struct spi_mcux_config *config = dev->config;
	SPI_Type *base = config->base;
#endif

	spi_context_lock(&data->ctx, asynchronous, cb, userdata, spi_cfg);

	ret = spi_mcux_configure(dev, spi_cfg);
	if (ret) {
		goto out;
	}

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

	spi_context_cs_control(&data->ctx, true);

#ifdef CONFIG_DSPI_MCUX_EDMA
	DSPI_StopTransfer(base);
	DSPI_FlushFifo(base, true, true);
	DSPI_ClearStatusFlags(base, (uint32_t)kDSPI_AllStatusFlag);
	/* setup the tx buffer with end  */
	mcux_init_inner_buffer_with_cmd(dev, 0);
	mcux_spi_context_data_update(dev);
	if (config->is_dma_chn_shared) {
		data->transfer_len = data->frame_size >> 3;
	} else {
		data->transfer_len = data->ctx.tx_len > data->ctx.rx_len ?
					     data->ctx.rx_len :
					     data->ctx.tx_len;
	}
	data->tx_transfer_count = 0;
	data->rx_transfer_count = 0;
	configure_dma(dev);
#endif

	ret = spi_mcux_transfer_next_packet(dev);
	if (ret) {
		goto out;
	}

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
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, false, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int spi_mcux_transceive_async(const struct device *dev,
				     const struct spi_config *spi_cfg,
				     const struct spi_buf_set *tx_bufs,
				     const struct spi_buf_set *rx_bufs,
				     spi_callback_t cb,
				     void *userdata)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, true, cb, userdata);
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
	int err;
	struct spi_mcux_data *data = dev->data;
	const struct spi_mcux_config *config = dev->config;

#ifdef CONFIG_DSPI_MCUX_EDMA
	enum dma_channel_filter spi_filter = DMA_CHANNEL_NORMAL;
	const struct device *dma_dev;

	dma_dev = data->rx_dma_config.dma_dev;
	data->rx_dma_config.dma_channel =
	  dma_request_channel(dma_dev, (void *)&spi_filter);
	dma_dev = data->tx_dma_config.dma_dev;
	data->tx_dma_config.dma_channel =
	  dma_request_channel(dma_dev, (void *)&spi_filter);
#else
	config->irq_config_func(dev);
#endif
	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err != 0) {
		return err;
	}

	data->dev = dev;

	err = spi_context_cs_configure_all(&data->ctx);
	if (err < 0) {
		return err;
	}

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


/* if a then b otherwise return 1 */
#define _UTIL_AND2(a, b) COND_CODE_1(UTIL_BOOL(a), (b), (1))

#ifdef CONFIG_DSPI_MCUX_EDMA

#define TX_BUFFER(id)							\
	static uint32_t							\
		edma_tx_buffer_##id[CONFIG_MCUX_DSPI_BUFFER_SIZE >> 2];	\
	static struct spi_buf spi_edma_tx_buffer_##id = {		\
		.buf = edma_tx_buffer_##id,				\
		.len = CONFIG_MCUX_DSPI_BUFFER_SIZE,			\
	}

#define RX_BUFFER(id)							\
	static uint32_t							\
		edma_rx_buffer_##id[CONFIG_MCUX_DSPI_BUFFER_SIZE >> 2];	\
	static struct spi_buf spi_edma_rx_buffer_##id = {		\
		.buf = edma_rx_buffer_##id,				\
		.len = CONFIG_MCUX_DSPI_BUFFER_SIZE,			\
	}

#define TX_DMA_CONFIG(id)						\
	.inner_tx_buffer = &spi_edma_tx_buffer_##id,			\
	.tx_dma_config = {						\
		.dma_dev =						\
		    DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(id, tx)),	\
		.dma_cfg = {						\
			.source_burst_length = 4,			\
			.dest_burst_length = 4,				\
			.source_data_size = 4,				\
			.dest_data_size = 4,				\
			.dma_callback = dma_callback,			\
			.complete_callback_en = 1,			\
			.error_callback_en = 1,				\
			.block_count = 1,				\
			.head_block = &spi_mcux_data_##id.tx_dma_block,	\
			.channel_direction = MEMORY_TO_PERIPHERAL,	\
			.dma_slot = DT_INST_DMAS_CELL_BY_NAME(		\
				id, tx, source),			\
		},							\
	},

#define RX_DMA_CONFIG(id)						\
	.inner_rx_buffer = &spi_edma_rx_buffer_##id,			\
	.rx_dma_config = {						\
		.dma_dev =						\
		    DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(id, rx)),	\
		.dma_cfg = {						\
			.source_burst_length = 2,			\
			.dest_burst_length = 2,				\
			.source_data_size = 2,				\
			.dest_data_size = 2,				\
			.dma_callback = dma_callback,			\
			.complete_callback_en = 1,			\
			.error_callback_en = 1,				\
			.block_count =					\
			_UTIL_AND2(DT_INST_NODE_HAS_PROP(		\
				id, nxp_rx_tx_chn_share), 2),		\
			.head_block = &spi_mcux_data_##id.rx_dma_block,	\
			.channel_direction = PERIPHERAL_TO_MEMORY,	\
			.dma_slot = DT_INST_DMAS_CELL_BY_NAME(		\
				id, rx, source),			\
		},							\
	},
#else
#define TX_BUFFER(id)
#define RX_BUFFER(id)
#define TX_DMA_CONFIG(id)
#define RX_DMA_CONFIG(id)

#endif

#define SPI_MCUX_DSPI_DEVICE(id)					\
	PINCTRL_DT_INST_DEFINE(id);					\
	static void spi_mcux_config_func_##id(const struct device *dev);\
	TX_BUFFER(id);							\
	RX_BUFFER(id);							\
	static struct spi_mcux_data spi_mcux_data_##id = {		\
		SPI_CONTEXT_BASE_INIT(spi_mcux_data_##id, ctx),		\
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(id), ctx)	\
		TX_DMA_CONFIG(id) RX_DMA_CONFIG(id)			\
	};								\
	static const struct spi_mcux_config spi_mcux_config_##id = {	\
		.base = (SPI_Type *)DT_INST_REG_ADDR(id),		\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(id)),	\
		.clock_subsys = 					\
		(clock_control_subsys_t)DT_INST_CLOCKS_CELL(id, name),	\
		.irq_config_func = spi_mcux_config_func_##id,		\
		.pcs_sck_delay =					\
		    DT_INST_PROP_OR(id, pcs_sck_delay, 0),		\
		.sck_pcs_delay =					\
		    DT_INST_PROP_OR(id, sck_pcs_delay, 0),		\
		.transfer_delay =					\
		    DT_INST_PROP_OR(id, transfer_delay, 0),		\
		.which_ctar =						\
		    DT_INST_PROP_OR(id, ctar, 0),			\
		.samplePoint =						\
		    DT_INST_PROP_OR(id, sample_point, 0),		\
		.enable_continuous_sck =					\
		    DT_INST_PROP(id, continuous_sck),			\
		.enable_rxfifo_overwrite =				\
		    DT_INST_PROP(id, rx_fifo_overwrite),		\
		.enable_modified_timing_format =				\
		    DT_INST_PROP(id, modified_timing_format),		\
		.is_dma_chn_shared =					\
		    DT_INST_PROP(id, nxp_rx_tx_chn_share),		\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(id),		\
	};								\
	DEVICE_DT_INST_DEFINE(id,					\
			    &spi_mcux_init,				\
			    NULL,					\
			    &spi_mcux_data_##id,			\
			    &spi_mcux_config_##id,			\
			    POST_KERNEL,				\
			    CONFIG_SPI_INIT_PRIORITY,		\
			    &spi_mcux_driver_api);			\
	static void spi_mcux_config_func_##id(const struct device *dev)	\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(id),				\
			    DT_INST_IRQ(id, priority),			\
			    spi_mcux_isr, DEVICE_DT_INST_GET(id),	\
			    0);						\
		irq_enable(DT_INST_IRQN(id));				\
	}

DT_INST_FOREACH_STATUS_OKAY(SPI_MCUX_DSPI_DEVICE)
