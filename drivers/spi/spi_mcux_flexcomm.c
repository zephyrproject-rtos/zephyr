/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2017, 2019, 2025, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpc_spi

#include <errno.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi/rtio.h>
#include <zephyr/drivers/clock_control.h>
#include <fsl_spi.h>
#include <zephyr/logging/log.h>
#ifdef CONFIG_SPI_MCUX_FLEXCOMM_DMA
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_mcux_lpc.h>
#endif
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys_clock.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/reset.h>

#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>

LOG_MODULE_REGISTER(spi_mcux_flexcomm, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"

#define SPI_CHIP_SELECT_COUNT	4
#define SPI_MAX_DATA_WIDTH	16
#define SPI_MIN_DATA_WIDTH	4

struct spi_mcux_config {
	SPI_Type *base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
#ifndef CONFIG_SPI_MCUX_FLEXCOMM_DMA
	void (*irq_config_func)(const struct device *dev);
#endif
	uint32_t pre_delay;
	uint32_t post_delay;
	uint32_t frame_delay;
	uint32_t transfer_delay;
	uint32_t def_char;
	const struct pinctrl_dev_config *pincfg;
	const struct reset_dt_spec reset;
};

#ifdef CONFIG_SPI_MCUX_FLEXCOMM_DMA
#define SPI_MCUX_FLEXCOMM_DMA_ERROR_FLAG	0x01
#define SPI_MCUX_FLEXCOMM_DMA_RX_DONE_FLAG	0x02
#define SPI_MCUX_FLEXCOMM_DMA_TX_DONE_FLAG	0x04
#define SPI_MCUX_FLEXCOMM_DMA_DONE_FLAG		\
	(SPI_MCUX_FLEXCOMM_DMA_RX_DONE_FLAG | SPI_MCUX_FLEXCOMM_DMA_TX_DONE_FLAG)

struct stream {
	const struct device *dma_dev;
	uint32_t channel; /* stores the channel for dma */
	struct dma_config dma_cfg;
	struct dma_block_config dma_blk_cfg[CONFIG_SPI_MCUX_FLEXCOMM_DMA_MAX_BLOCKS];
};
#endif

struct spi_mcux_data {
	const struct device *dev;
	spi_master_handle_t handle;
	struct spi_context ctx;
	size_t transfer_len;
	uint16_t word_size_bytes;
	uint16_t word_size_bits;
#ifdef CONFIG_SPI_MCUX_FLEXCOMM_DMA
	volatile uint32_t status_flags;
	struct stream dma_rx;
	struct stream dma_tx;
	/* dummy value used for transferring NOP when tx buf is null */
	uint32_t dummy_tx_buffer;
	/* Used to send the last word */
	uint32_t last_word;
#endif
};

static bool force_reconfig;

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
		spi_context_complete(&data->ctx, dev, 0);
		spi_context_release(&data->ctx, 0);
		pm_policy_device_power_lock_put(dev);
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
		spi_context_cs_control(&data->ctx, false);
		spi_context_complete(&data->ctx, dev, -EIO);
		pm_policy_device_power_lock_put(dev);
	}
}

#ifndef CONFIG_SPI_MCUX_FLEXCOMM_DMA
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

	spi_context_update_tx(&data->ctx, data->word_size_bytes, data->transfer_len);
	spi_context_update_rx(&data->ctx, data->word_size_bytes, data->transfer_len);

	spi_mcux_transfer_next_packet(data->dev);
}
#endif /* CONFIG_SPI_MCUX_FLEXCOMM_DMA */

static uint8_t spi_clock_cycles(uint32_t delay_ns, uint32_t sck_frequency_hz)
{
	/* Convert delay_ns to an integer number of clock cycles of frequency
	 * sck_frequency_hz. The maximum delay is 15 clock cycles.
	 */
	uint8_t delay_cycles = (uint64_t)delay_ns * sck_frequency_hz / NSEC_PER_SEC;

	delay_cycles = MIN(delay_cycles, 15);

	return delay_cycles;
}

static int spi_mcux_configure(const struct device *dev,
			      const struct spi_config *spi_cfg)
{
	const struct spi_mcux_config *config = dev->config;
	struct spi_mcux_data *data = dev->data;
	SPI_Type *base = config->base;
	uint32_t clock_freq;

	if ((spi_context_configured(&data->ctx, spi_cfg)) && (!force_reconfig)) {
		/* This configuration is already in use */
		return 0;
	}

	force_reconfig = false;

	if (spi_cfg->operation & SPI_HALF_DUPLEX) {
		LOG_ERR("Half-duplex not supported");
		return -ENOTSUP;
	}

	/*
	 * Do master or slave initialisation, depending on the
	 * mode requested.
	 */
	if (SPI_OP_MODE_GET(spi_cfg->operation) == SPI_OP_MODE_MASTER) {
		spi_master_config_t master_config;

		SPI_MasterGetDefaultConfig(&master_config);

		if (!device_is_ready(config->clock_dev)) {
			LOG_ERR("clock control device not ready");
			return -ENODEV;
		}

		/* Get the clock frequency */
		if (clock_control_get_rate(config->clock_dev,
					   config->clock_subsys, &clock_freq)) {
			return -EINVAL;
		}

		if (spi_cfg->slave > SPI_CHIP_SELECT_COUNT) {
			LOG_ERR("Slave %d is greater than %d",
				    spi_cfg->slave, SPI_CHIP_SELECT_COUNT);
			return -EINVAL;
		}

		master_config.sselNum = spi_cfg->slave;
		master_config.sselPol = kSPI_SpolActiveAllLow;
		master_config.dataWidth = data->word_size_bits - 1;

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

		spi_delay_config_t *delayConfig = &master_config.delayConfig;

		delayConfig->preDelay = spi_clock_cycles(config->pre_delay,
							spi_cfg->frequency);
		delayConfig->postDelay = spi_clock_cycles(config->post_delay,
							spi_cfg->frequency);
		delayConfig->frameDelay = spi_clock_cycles(config->frame_delay,
							spi_cfg->frequency);
		delayConfig->transferDelay = spi_clock_cycles(config->transfer_delay,
							spi_cfg->frequency);

		SPI_MasterInit(base, &master_config, clock_freq);

#ifndef CONFIG_SPI_MCUX_FLEXCOMM_DMA
		SPI_SetDummyData(base, (uint8_t)config->def_char);

		SPI_MasterTransferCreateHandle(base, &data->handle,
					     spi_mcux_transfer_callback, data);
#endif

		data->ctx.config = spi_cfg;
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
		slave_config.dataWidth = data->word_size_bits - 1;

		SPI_SlaveInit(base, &slave_config);

#ifndef CONFIG_SPI_MCUX_FLEXCOMM_DMA
		SPI_SetDummyData(base, (uint8_t)config->def_char);

		SPI_SlaveTransferCreateHandle(base, &data->handle,
					      spi_mcux_transfer_callback, data);
#endif

		data->ctx.config = spi_cfg;
	}

	return 0;
}

#ifdef CONFIG_SPI_MCUX_FLEXCOMM_DMA
/* Dummy buffer used as a sink when rc buf is null */
static uint32_t dummy_rx_buffer;

/* This function is executed in the interrupt context */
static void spi_mcux_dma_callback(const struct device *dev, void *arg,
			 uint32_t channel, int status)
{
	/* arg directly holds the spi device */
	const struct device *spi_dev = arg;
	struct spi_mcux_data *data = spi_dev->data;

	if (status < 0) {
		LOG_ERR("DMA callback error with channel %d.", channel);
		data->status_flags |= SPI_MCUX_FLEXCOMM_DMA_ERROR_FLAG;
	} else {
		/* identify the origin of this callback */
		if (channel == data->dma_tx.channel) {
			/* this part of the transfer ends */
			data->status_flags |= SPI_MCUX_FLEXCOMM_DMA_TX_DONE_FLAG;
		} else if (channel == data->dma_rx.channel) {
			/* The RX DMA interrupt will trigger once all of the data is transferred, so
			 * we use that to call context complete.
			 */
			data->status_flags |= SPI_MCUX_FLEXCOMM_DMA_RX_DONE_FLAG;
			spi_context_cs_control(&data->ctx, false);
			spi_context_complete(&data->ctx, spi_dev, 0);
			pm_policy_device_power_lock_put(dev);
		} else {
			LOG_ERR("DMA callback channel %d is not valid.",
								channel);
			data->status_flags |= SPI_MCUX_FLEXCOMM_DMA_ERROR_FLAG;
			spi_context_cs_control(&data->ctx, false);
			spi_context_complete(&data->ctx, spi_dev, -EIO);
			pm_policy_device_power_lock_put(dev);
		}
	}
}

static uint32_t spi_mcux_get_tx_word(uint32_t value, const struct spi_config *spi_cfg,
				     uint8_t word_size)
{
	value |= ((uint32_t)SPI_DEASSERT_ALL &
		  (~(uint32_t)SPI_DEASSERTNUM_SSEL((uint32_t)spi_cfg->slave)));
	/* set width of data - range asserted at entry */
	value |= SPI_FIFOWR_LEN(word_size - 1);
	return value;
}

static uint32_t spi_mcux_get_last_tx_word(const struct spi_config *spi_cfg, const uint8_t *buf,
					  size_t len, uint32_t def_char, uint8_t word_size)
{
	uint32_t value = def_char;

	/* Buffer will be null if TX is sending dummy data. In this case, we don't
	 * need to copy any data from the buffer into the dummy word.
	 */
	if (buf != NULL) {
		if (word_size > 8) {
			assert(len > 1);
			value = (((uint32_t)buf[len - 1U] << 8U) | (buf[len - 2U]));
		} else {
			value = buf[len - 1U];
		}
	}
	value |= (uint32_t)SPI_FIFOWR_EOT_MASK;

	return spi_mcux_get_tx_word(value, spi_cfg, word_size);
}

static int spi_mcux_dma_tx_load(const struct device *dev, const struct spi_config *spi_cfg,
				const uint8_t *buf, size_t len, size_t dma_block_num,
				bool last_packet)
{
	const struct spi_mcux_config *config = dev->config;
	struct spi_mcux_data *data = dev->data;
	SPI_Type *base = config->base;
	struct dma_block_config *blk_cfg;

	/* remember active TX DMA channel (used in callback) */
	struct stream *stream = &data->dma_tx;

	/* prepare the block for this TX DMA channel */
	blk_cfg = &stream->dma_blk_cfg[dma_block_num];
	/* tx direction has memory as source and periph as dest. */
	blk_cfg->dest_address = (uint32_t)&base->FIFOWR;
	blk_cfg->dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	blk_cfg->dest_scatter_en = 0;
	blk_cfg->source_gather_en = 0;
	if (last_packet) {
		data->last_word = spi_mcux_get_last_tx_word(spi_cfg, buf, len, config->def_char,
							    data->word_size_bits);
		blk_cfg->source_address = (uint32_t)&data->last_word;
		blk_cfg->source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		blk_cfg->block_size = sizeof(uint32_t);
		blk_cfg->next_block = NULL;
	} else {
		blk_cfg->block_size = len;
		blk_cfg->next_block = blk_cfg + 1;
		if (buf) {
			blk_cfg->source_address = (uint32_t)buf;
			blk_cfg->source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		} else {
			data->dummy_tx_buffer = 0;
			blk_cfg->source_address = (uint32_t)&data->dummy_tx_buffer;
			blk_cfg->source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		}
	}
	return EXIT_SUCCESS;
}

static int spi_mcux_dma_tx_start(const struct device *dev, const struct spi_config *spi_cfg)
{
	const struct spi_mcux_config *config = dev->config;
	struct spi_mcux_data *data = dev->data;
	SPI_Type *base = config->base;
	struct stream *stream = &data->dma_tx;
	int ret;

	/* direction is given by the DT */
	stream->dma_cfg.head_block = &stream->dma_blk_cfg[0];
	/* give the client dev as arg, as the callback comes from the dma */
	stream->dma_cfg.user_data = (struct device *)dev;
	/* pass our client origin to the dma: data->dma_tx.dma_channel */
	ret = dma_config(data->dma_tx.dma_dev, data->dma_tx.channel,
			&stream->dma_cfg);
	/* the channel is the actual stream from 0 */
	if (ret != 0) {
		return ret;
	}

	/* Setup the control info.
	 * Halfword writes to just the control bits (offset 0xE22) doesn't push
	 * anything into the FIFO. And the data access type of control bits must
	 * be uint16_t, byte writes or halfword writes to FIFOWR will push the
	 * data and the current control bits into the FIFO.
	 */
	uint32_t tmp_data = spi_mcux_get_tx_word(config->def_char, spi_cfg, data->word_size_bits);
	*((uint16_t *)((uint32_t)&base->FIFOWR) + 1) = (uint16_t)(tmp_data >> 16U);

	/* gives the request ID */
	return dma_start(data->dma_tx.dma_dev, data->dma_tx.channel);
}

static int spi_mcux_dma_rx_load(const struct device *dev, uint8_t *buf, size_t len,
				size_t dma_block_num, bool last_packet)
{
	const struct spi_mcux_config *config = dev->config;
	struct spi_mcux_data *data = dev->data;
	SPI_Type *base = config->base;
	struct dma_block_config *blk_cfg;

	/* retrieve active RX DMA channel (used in callback) */
	struct stream *stream = &data->dma_rx;

	/* prepare the block for this RX DMA channel */
	blk_cfg = &stream->dma_blk_cfg[dma_block_num];
	blk_cfg->block_size = len;
	blk_cfg->dest_scatter_en = 0;
	if (last_packet) {
		blk_cfg->next_block = NULL;
	} else {
		blk_cfg->next_block = blk_cfg + 1;
	}
	/* rx direction has periph as source and mem as dest. */
	blk_cfg->source_address = (uint32_t)&base->FIFORD;
	blk_cfg->source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	blk_cfg->source_gather_en = 0;
	if (buf == NULL) {
		/* if rx buff is null, then write data to dummy address. */
		blk_cfg->dest_address = (uint32_t)&dummy_rx_buffer;
		blk_cfg->dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	} else {
		blk_cfg->dest_address = (uint32_t)buf;
		blk_cfg->dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	}
	return EXIT_SUCCESS;
}

static int spi_mcux_dma_rx_start(const struct device *dev)
{
	struct spi_mcux_data *data = dev->data;
	struct stream *stream = &data->dma_rx;
	int ret;

	/* direction is given by the DT */
	stream->dma_cfg.head_block = &stream->dma_blk_cfg[0];
	stream->dma_cfg.user_data = (struct device *)dev;

	/* pass our client origin to the dma: data->dma_rx.channel */
	ret = dma_config(data->dma_rx.dma_dev, data->dma_rx.channel,
			&stream->dma_cfg);
	/* the channel is the actual stream from 0 */
	if (ret != 0) {
		return ret;
	}

	/* gives the request ID */
	return dma_start(data->dma_rx.dma_dev, data->dma_rx.channel);
}

static int spi_mcux_dma_transfer(const struct device *dev, const struct spi_config *spi_cfg)
{
	struct spi_mcux_data *data = dev->data;
	const size_t data_size = data->word_size_bytes;
	size_t dma_block = 0;
	size_t block_length;
	int ret;
	bool last_packet = false;

	/* Setup DMA tx/rx data size based on spi word size.*/
	data->dma_rx.dma_cfg.source_data_size = data_size;
	data->dma_rx.dma_cfg.dest_data_size = data_size;
	data->dma_tx.dma_cfg.source_data_size = data_size;
	data->dma_tx.dma_cfg.dest_data_size = data_size;

	data->status_flags = 0;

	/* Parse all data to be sent into chained DMA blocks. */
	while (1) {
		block_length = spi_context_max_continuous_chunk(&data->ctx);
		assert(block_length >= data_size);
		if (data->ctx.tx_count <= 1 && data->ctx.rx_count <= 1 &&
		    block_length == spi_context_longest_current_buf(&data->ctx)) {
			/* On the last buffer. First send all but the last word, then when only one
			 * word is remaining, load the last buffer with that word so it can be set
			 * to release the CS line.
			 */
			if (block_length > data_size) {
				block_length -= data_size;
			} else {
				/* There is only one transfer left. */
				last_packet = true;
			}
		}
		ret = spi_mcux_dma_rx_load(dev, data->ctx.rx_buf, block_length, dma_block,
					   last_packet);
		if (ret) {
			LOG_ERR("could not load rx data to dma: %d", ret);
			return ret;
		}
		ret = spi_mcux_dma_tx_load(dev, spi_cfg, data->ctx.tx_buf, block_length, dma_block,
					   last_packet);
		if (ret) {
			LOG_ERR("could not load tx data to dma: %d", ret);
			return ret;
		}
		spi_context_update_rx(&data->ctx, data_size, block_length);
		spi_context_update_tx(&data->ctx, data_size, block_length);
		/* Increment block count before exit so the last block is counted for dma_cfg. */
		dma_block++;
		if (last_packet) {
			assert(spi_context_total_rx_len(&data->ctx) == 0);
			assert(spi_context_total_tx_len(&data->ctx) == 0);
			break;
		}
		if (dma_block == CONFIG_SPI_MCUX_FLEXCOMM_DMA_MAX_BLOCKS) {
			LOG_ERR("spi xfer exceeds dma block allocation");
			return -ENOMEM;
		}
	}

	/* Update dma_cfg with actual blocks used. */
	data->dma_rx.dma_cfg.block_count = dma_block;
	data->dma_tx.dma_cfg.block_count = dma_block;

	ret = spi_mcux_dma_rx_start(dev);
	if (ret) {
		LOG_ERR("could not start dma rx: %d", ret);
		return -EIO;
	}

	ret = spi_mcux_dma_tx_start(dev, spi_cfg);
	if (ret) {
		LOG_ERR("could not start dma tx: %d", ret);
		return -EIO;
	}
	return EXIT_SUCCESS;
}

static void spi_mcux_transfer_one_word(const struct device *dev, const struct spi_config *spi_cfg)
{
	/* don't use the SDK, which is overkill for a one word transfer */
	const struct spi_mcux_config *config = dev->config;
	struct spi_mcux_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	SPI_Type *base = config->base;
	uint32_t tmp32;

	tmp32 = spi_mcux_get_last_tx_word(spi_cfg, ctx->tx_buf, 1, config->def_char,
					  data->word_size_bits);

	/* wait for TXFIFO to not be full */
	while ((base->FIFOSTAT & SPI_FIFOSTAT_TXNOTFULL_MASK) == 0) {
	}

	/* txFIFO is not full */
	base->FIFOWR = tmp32;

	/* wait for RXFIFO to not be empty */
	while ((base->FIFOSTAT & SPI_FIFOSTAT_RXNOTEMPTY_MASK) == 0) {
	}

	/* rxFIFO is not empty */
	tmp32 = base->FIFORD;

	/* copy to user buffer if given one */
	if (ctx->rx_len) {
		assert(ctx->rx_buf);
		ctx->rx_buf[0] = (uint8_t)tmp32;
		if (data->word_size_bits > 8) {
			ctx->rx_buf[1] = (uint8_t)(tmp32 >> 8);
		}
	}

	/* wait if TX FIFO of previous transfer is not empty */
	while ((base->FIFOSTAT & SPI_FIFOSTAT_TXEMPTY_MASK) == 0) {
	}
}

static int transceive_dma(const struct device *dev,
		      const struct spi_config *spi_cfg,
		      const struct spi_buf_set *tx_bufs,
		      const struct spi_buf_set *rx_bufs,
		      bool asynchronous,
		      spi_callback_t cb,
		      void *userdata)
{
	const struct spi_mcux_config *config = dev->config;
	struct spi_mcux_data *data = dev->data;
	SPI_Type *base = config->base;
	int ret;
	uint8_t word_size = (uint8_t)SPI_WORD_SIZE_GET(spi_cfg->operation);

	if (word_size > SPI_MAX_DATA_WIDTH) {
		LOG_ERR("Word size %d is greater than %d", word_size, SPI_MAX_DATA_WIDTH);
		return -EINVAL;
	}

	pm_policy_device_power_lock_get(dev);

	spi_context_lock(&data->ctx, asynchronous, cb, userdata, spi_cfg);

	data->word_size_bits = word_size;
	data->word_size_bytes = (word_size > 8) ? (sizeof(uint16_t)) : (sizeof(uint8_t));
	ret = spi_mcux_configure(dev, spi_cfg);
	if (ret) {
		goto out;
	}

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

	spi_context_cs_control(&data->ctx, true);

	/* Clear FIFOs and any previous errors before transfer */
	base->FIFOCFG |= SPI_FIFOCFG_EMPTYTX_MASK | SPI_FIFOCFG_EMPTYRX_MASK;
	base->FIFOSTAT |= SPI_FIFOSTAT_TXERR_MASK | SPI_FIFOSTAT_RXERR_MASK;

	/* Check if this transaction is only sending one word of data. If this is the case, it is
	 * less work to load that word into the FIFO instead of setting up the DMA to write one
	 * word. This also avoids the edge case where the FIFOWR bits cannot be set by a chained
	 * DMA descriptor, because there is only one DMA descriptor.
	 */
	if (data->ctx.tx_count <= 1 && data->ctx.rx_count <= 1 &&
	    spi_context_longest_current_buf(&data->ctx) == data->word_size_bytes) {
		/* Disable DMATX/RX */
		base->FIFOCFG &= ~(SPI_FIFOCFG_DMARX_MASK | SPI_FIFOCFG_DMATX_MASK);

		spi_mcux_transfer_one_word(dev, spi_cfg);

		spi_context_update_tx(&data->ctx, data->word_size_bytes, data->transfer_len);
		spi_context_update_rx(&data->ctx, data->word_size_bytes, data->transfer_len);
		spi_context_cs_control(&data->ctx, false);
		spi_context_complete(&data->ctx, dev, 0);
		/* If asynchronous was true, set to false since transfer is done and we
		 * want to release the power lock below.
		 */
		asynchronous = false;
	} else {
		/* Enable DMATX/RX. */
		base->FIFOCFG |= SPI_FIFOCFG_DMARX_MASK | SPI_FIFOCFG_DMATX_MASK;
		ret = spi_mcux_dma_transfer(dev, spi_cfg);
		if (ret) {
			spi_context_cs_control(&data->ctx, false);
			goto out;
		}
	}

	/* if asynchronous is true, spi_context_wait_for_completion() just returns 0
	 * and spi_context_release() is a noop
	 */
	ret = spi_context_wait_for_completion(&data->ctx);

out:
	spi_context_release(&data->ctx, ret);

	if (!asynchronous || (ret != 0)) {
		/* Only release the power lock if synchronous, or if an error occurred.
		 * For asynchronous case, the power_lock is released in
		 * when the transfer is done in the dma completion callback
		 */
		pm_policy_device_power_lock_put(dev);
	}

	return ret;
}

#endif

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
	uint8_t word_size = (uint8_t)SPI_WORD_SIZE_GET(spi_cfg->operation);

	if (word_size > SPI_MAX_DATA_WIDTH) {
		LOG_ERR("Word size %d is greater than %d", word_size, SPI_MAX_DATA_WIDTH);
		return -EINVAL;
	}

	if (word_size < SPI_MIN_DATA_WIDTH) {
		LOG_ERR("Word size %d is less than %d", word_size, SPI_MIN_DATA_WIDTH);
		return -EINVAL;
	}

	pm_policy_device_power_lock_get(dev);

	spi_context_lock(&data->ctx, asynchronous, cb, userdata, spi_cfg);

	data->word_size_bits = word_size;
	data->word_size_bytes = (word_size > 8) ? (sizeof(uint16_t)) : (sizeof(uint8_t));
	ret = spi_mcux_configure(dev, spi_cfg);
	if (ret) {
		goto out;
	}

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

	spi_context_cs_control(&data->ctx, true);

	spi_mcux_transfer_next_packet(dev);

	return spi_context_wait_for_completion(&data->ctx);

out:
	spi_context_release(&data->ctx, ret);

	pm_policy_device_power_lock_put(dev);

	return ret;
}

static int spi_mcux_transceive(const struct device *dev,
			       const struct spi_config *spi_cfg,
			       const struct spi_buf_set *tx_bufs,
			       const struct spi_buf_set *rx_bufs)
{
#ifdef CONFIG_SPI_MCUX_FLEXCOMM_DMA
	return transceive_dma(dev, spi_cfg, tx_bufs, rx_bufs, false, NULL, NULL);
#endif
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
#ifdef CONFIG_SPI_MCUX_FLEXCOMM_DMA
	return transceive_dma(dev, spi_cfg, tx_bufs, rx_bufs, true, cb, userdata);
#endif

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

static int spi_mcux_init_common(const struct device *dev)
{
	const struct spi_mcux_config *config = dev->config;
	struct spi_mcux_data *data = dev->data;
	int err = 0;

	if (!device_is_ready(config->reset.dev)) {
		LOG_ERR("Reset device not ready");
		return -ENODEV;
	}

	err = reset_line_toggle(config->reset.dev, config->reset.id);
	if (err) {
		return err;
	}

#ifndef CONFIG_SPI_MCUX_FLEXCOMM_DMA
	config->irq_config_func(dev);
#endif

	data->dev = dev;

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}

#ifdef CONFIG_SPI_MCUX_FLEXCOMM_DMA
	if (!device_is_ready(data->dma_tx.dma_dev)) {
		LOG_ERR("%s device is not ready", data->dma_tx.dma_dev->name);
		return -ENODEV;
	}

	if (!device_is_ready(data->dma_rx.dma_dev)) {
		LOG_ERR("%s device is not ready", data->dma_rx.dma_dev->name);
		return -ENODEV;
	}
#endif /* CONFIG_SPI_MCUX_FLEXCOMM_DMA */


	err = spi_context_cs_configure_all(&data->ctx);
	if (err < 0) {
		return err;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static int spi_mcux_flexcomm_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
	    /*This flag is used to prevent configuration optimiztions
	     * after exiting PM3 on spi_mcux_configure()
	     */
		force_reconfig = true;
		break;
	case PM_DEVICE_ACTION_TURN_ON:
		spi_mcux_init_common(dev);
		break;
	default:
		return -ENOTSUP;
	}
	return 0;
}

static int spi_mcux_init(const struct device *dev)
{
	/* Rest of the init is done from the PM_DEVICE_TURN_ON action
	 * which is invoked by pm_device_driver_init().
	 */
	return pm_device_driver_init(dev, spi_mcux_flexcomm_pm_action);
}

static DEVICE_API(spi, spi_mcux_driver_api) = {
	.transceive = spi_mcux_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_mcux_transceive_async,
#endif
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
	.release = spi_mcux_release,
};

#ifndef CONFIG_SPI_MCUX_FLEXCOMM_DMA
#define SPI_MCUX_FLEXCOMM_IRQ_HANDLER_DECL(id)				\
	static void spi_mcux_config_func_##id(const struct device *dev)
#define SPI_MCUX_FLEXCOMM_IRQ_HANDLER_FUNC(id)				\
	.irq_config_func = spi_mcux_config_func_##id,
#define SPI_MCUX_FLEXCOMM_IRQ_HANDLER(id)                                                          \
	static void spi_mcux_config_func_##id(const struct device *dev)                            \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(id), DT_INST_IRQ(id, priority), spi_mcux_isr,             \
			    DEVICE_DT_INST_GET(id), 0);                                            \
		irq_enable(DT_INST_IRQN(id));                                                      \
	}
#else
#define SPI_MCUX_FLEXCOMM_IRQ_HANDLER_DECL(id)
#define SPI_MCUX_FLEXCOMM_IRQ_HANDLER_FUNC(id)
#define SPI_MCUX_FLEXCOMM_IRQ_HANDLER(id)
#endif

#ifndef CONFIG_SPI_MCUX_FLEXCOMM_DMA
#define SPI_DMA_CHANNELS(id)
#else
#define SPI_DMA_CHANNELS(id)				\
	.dma_tx = {						\
		.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(id, tx)), \
		.channel =					\
			DT_INST_DMAS_CELL_BY_NAME(id, tx, channel),	\
		.dma_cfg = {						\
			.channel_direction = LPC_DMA_SPI_MCUX_FLEXCOMM_TX,	\
			.dma_callback = spi_mcux_dma_callback,		\
			.block_count = 2,				\
		}							\
	},								\
	.dma_rx = {						\
		.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(id, rx)), \
		.channel =					\
			DT_INST_DMAS_CELL_BY_NAME(id, rx, channel),	\
		.dma_cfg = {				\
			.channel_direction = PERIPHERAL_TO_MEMORY,	\
			.dma_callback = spi_mcux_dma_callback,		\
			.block_count = 1,		\
		}							\
	}

#endif

#define SPI_MCUX_FLEXCOMM_DEVICE(id)					\
	SPI_MCUX_FLEXCOMM_IRQ_HANDLER_DECL(id);			\
	PINCTRL_DT_INST_DEFINE(id);					\
	static const struct spi_mcux_config spi_mcux_config_##id = {	\
		.base =							\
		(SPI_Type *)DT_INST_REG_ADDR(id),			\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(id)),	\
		.clock_subsys =					\
		(clock_control_subsys_t)DT_INST_CLOCKS_CELL(id, name),\
		SPI_MCUX_FLEXCOMM_IRQ_HANDLER_FUNC(id)			\
		.pre_delay = DT_INST_PROP_OR(id, pre_delay, 0),		\
		.post_delay = DT_INST_PROP_OR(id, post_delay, 0),		\
		.frame_delay = DT_INST_PROP_OR(id, frame_delay, 0),		\
		.transfer_delay = DT_INST_PROP_OR(id, transfer_delay, 0),		\
		.def_char = DT_INST_PROP_OR(id, def_char, 0),		\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(id),		\
		.reset = RESET_DT_SPEC_INST_GET(id),			\
	};								\
	static struct spi_mcux_data spi_mcux_data_##id = {		\
		SPI_CONTEXT_INIT_LOCK(spi_mcux_data_##id, ctx),		\
		SPI_CONTEXT_INIT_SYNC(spi_mcux_data_##id, ctx),		\
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(id), ctx)	\
		SPI_DMA_CHANNELS(id)		\
	};								\
	PM_DEVICE_DT_INST_DEFINE(id, spi_mcux_flexcomm_pm_action);	\
	SPI_DEVICE_DT_INST_DEFINE(id,					\
			    spi_mcux_init,				\
			    PM_DEVICE_DT_INST_GET(id),			\
			    &spi_mcux_data_##id,			\
			    &spi_mcux_config_##id,			\
			    POST_KERNEL,				\
			    CONFIG_SPI_INIT_PRIORITY,			\
			    &spi_mcux_driver_api);			\
	\
	SPI_MCUX_FLEXCOMM_IRQ_HANDLER(id)

DT_INST_FOREACH_STATUS_OKAY(SPI_MCUX_FLEXCOMM_DEVICE)
