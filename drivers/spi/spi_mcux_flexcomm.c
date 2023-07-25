/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright (c) 2017,2019, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpc_spi

#include <errno.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/clock_control.h>
#include <fsl_spi.h>
#include <zephyr/logging/log.h>
#ifdef CONFIG_SPI_MCUX_FLEXCOMM_DMA
#include <zephyr/drivers/dma.h>
#endif
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys_clock.h>
#include <zephyr/irq.h>

LOG_MODULE_REGISTER(spi_mcux_flexcomm, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"

#define SPI_CHIP_SELECT_COUNT	4
#define SPI_MAX_DATA_WIDTH	16

struct spi_mcux_config {
	SPI_Type *base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	void (*irq_config_func)(const struct device *dev);
	uint32_t pre_delay;
	uint32_t post_delay;
	uint32_t frame_delay;
	uint32_t transfer_delay;
	uint32_t def_char;
	const struct pinctrl_dev_config *pincfg;
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
	struct dma_block_config dma_blk_cfg[2];
};
#endif

struct spi_mcux_data {
	const struct device *dev;
	spi_master_handle_t handle;
	struct spi_context ctx;
	size_t transfer_len;
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
	uint32_t word_size;

	if (spi_context_configured(&data->ctx, spi_cfg)) {
		/* This configuration is already in use */
		return 0;
	}

	if (spi_cfg->operation & SPI_HALF_DUPLEX) {
		LOG_ERR("Half-duplex not supported");
		return -ENOTSUP;
	}

	word_size = SPI_WORD_SIZE_GET(spi_cfg->operation);
	if (word_size > SPI_MAX_DATA_WIDTH) {
		LOG_ERR("Word size %d is greater than %d",
			    word_size, SPI_MAX_DATA_WIDTH);
		return -EINVAL;
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

		SPI_SetDummyData(base, (uint8_t)config->def_char);

		SPI_MasterTransferCreateHandle(base, &data->handle,
					     spi_mcux_transfer_callback, data);

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
		slave_config.dataWidth = word_size - 1;

		SPI_SlaveInit(base, &slave_config);

		SPI_SetDummyData(base, (uint8_t)config->def_char);

		SPI_SlaveTransferCreateHandle(base, &data->handle,
					      spi_mcux_transfer_callback, data);

		data->ctx.config = spi_cfg;
	}

	return 0;
}

#ifdef CONFIG_SPI_MCUX_FLEXCOMM_DMA
/* Dummy buffer used as a sink when rc buf is null */
uint32_t dummy_rx_buffer;

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
			/* this part of the transfer ends */
			data->status_flags |= SPI_MCUX_FLEXCOMM_DMA_RX_DONE_FLAG;
		} else {
			LOG_ERR("DMA callback channel %d is not valid.",
								channel);
			data->status_flags |= SPI_MCUX_FLEXCOMM_DMA_ERROR_FLAG;
		}
	}

	spi_context_complete(&data->ctx, spi_dev, 0);
}


static void spi_mcux_prepare_txlastword(uint32_t *txLastWord,
				const uint8_t *buf, const struct spi_config *spi_cfg,
				size_t len)
{
	uint32_t word_size;

	word_size = SPI_WORD_SIZE_GET(spi_cfg->operation);

	if (word_size > 8) {
		*txLastWord = (((uint32_t)buf[len - 1U] << 8U) |
							(buf[len - 2U]));
	} else {
		*txLastWord = buf[len - 1U];
	}

	*txLastWord |= (uint32_t)SPI_FIFOWR_EOT_MASK;

	*txLastWord |= ((uint32_t)SPI_DEASSERT_ALL &
				(~(uint32_t)SPI_DEASSERTNUM_SSEL((uint32_t)spi_cfg->slave)));

	/* set width of data - range asserted at entry */
	*txLastWord |= SPI_FIFOWR_LEN(word_size - 1);
}

static void spi_mcux_prepare_txdummy(uint32_t *dummy, bool last_packet,
				const struct spi_config *spi_cfg)
{
	uint32_t word_size;

	word_size = SPI_WORD_SIZE_GET(spi_cfg->operation);

	if (last_packet) {
		*dummy |= (uint32_t)SPI_FIFOWR_EOT_MASK;
	}

	*dummy |= ((uint32_t)SPI_DEASSERT_ALL &
				(~(uint32_t)SPI_DEASSERTNUM_SSEL((uint32_t)spi_cfg->slave)));

	/* set width of data - range asserted at entry */
	*dummy |= SPI_FIFOWR_LEN(word_size - 1);
}

static int spi_mcux_dma_tx_load(const struct device *dev, const uint8_t *buf,
				 const struct spi_config *spi_cfg, size_t len, bool last_packet)
{
	const struct spi_mcux_config *cfg = dev->config;
	struct spi_mcux_data *data = dev->data;
	struct dma_block_config *blk_cfg;
	int ret;
	SPI_Type *base = cfg->base;
	uint32_t word_size;

	word_size = SPI_WORD_SIZE_GET(spi_cfg->operation);

	/* remember active TX DMA channel (used in callback) */
	struct stream *stream = &data->dma_tx;

	blk_cfg = &stream->dma_blk_cfg[0];

	/* prepare the block for this TX DMA channel */
	memset(blk_cfg, 0, sizeof(struct dma_block_config));

	/* tx direction has memory as source and periph as dest. */
	if (buf == NULL) {
		data->dummy_tx_buffer = 0;
		data->last_word = 0;
		spi_mcux_prepare_txdummy(&data->dummy_tx_buffer, last_packet, spi_cfg);

		if (last_packet  &&
		    ((word_size > 8) ? (len > 2U) : (len > 1U))) {
			spi_mcux_prepare_txdummy(&data->last_word, last_packet, spi_cfg);
			blk_cfg->source_gather_en = 1;
			blk_cfg->source_address = (uint32_t)&data->dummy_tx_buffer;
			blk_cfg->dest_address = (uint32_t)&base->FIFOWR;
			blk_cfg->block_size = (word_size > 8) ?
						(len - 2U) : (len - 1U);
			blk_cfg->next_block = &stream->dma_blk_cfg[1];
			blk_cfg->source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
			blk_cfg->dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;

			blk_cfg = &stream->dma_blk_cfg[1];

			/* prepare the block for this TX DMA channel */
			memset(blk_cfg, 0, sizeof(struct dma_block_config));
			blk_cfg->source_address = (uint32_t)&data->last_word;
			blk_cfg->dest_address = (uint32_t)&base->FIFOWR;
			blk_cfg->block_size = sizeof(uint32_t);
			blk_cfg->source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
			blk_cfg->dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		} else {
			blk_cfg->source_address = (uint32_t)&data->dummy_tx_buffer;
			blk_cfg->dest_address = (uint32_t)&base->FIFOWR;
			blk_cfg->block_size = len;
			blk_cfg->source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
			blk_cfg->dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		}
	} else {
		if (last_packet) {
			spi_mcux_prepare_txlastword(&data->last_word, buf, spi_cfg, len);
		}
		/* If last packet and data transfer frame is bigger then 1,
		 * use dma descriptor to send the last data.
		 */
		if (last_packet  &&
		    ((word_size > 8) ? (len > 2U) : (len > 1U))) {
			blk_cfg->source_gather_en = 1;
			blk_cfg->source_address = (uint32_t)buf;
			blk_cfg->dest_address = (uint32_t)&base->FIFOWR;
			blk_cfg->block_size = (word_size > 8) ?
						(len - 2U) : (len - 1U);
			blk_cfg->next_block = &stream->dma_blk_cfg[1];

			blk_cfg = &stream->dma_blk_cfg[1];

			/* prepare the block for this TX DMA channel */
			memset(blk_cfg, 0, sizeof(struct dma_block_config));
			blk_cfg->source_address = (uint32_t)&data->last_word;
			blk_cfg->dest_address = (uint32_t)&base->FIFOWR;
			blk_cfg->block_size = sizeof(uint32_t);
			blk_cfg->source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
			blk_cfg->dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		} else {
			blk_cfg->source_address = (uint32_t)buf;
			blk_cfg->dest_address = (uint32_t)&base->FIFOWR;
			blk_cfg->block_size = len;
		}
	}

	/* Enables the DMA request from SPI txFIFO */
	base->FIFOCFG |= SPI_FIFOCFG_DMATX_MASK;

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

	uint32_t tmpData = 0U;

	spi_mcux_prepare_txdummy(&tmpData, last_packet, spi_cfg);

	/* Setup the control info.
	 * Halfword writes to just the control bits (offset 0xE22) doesn't push
	 * anything into the FIFO. And the data access type of control bits must
	 * be uint16_t, byte writes or halfword writes to FIFOWR will push the
	 * data and the current control bits into the FIFO.
	 */
	if ((last_packet) &&
		((word_size > 8) ? (len == 2U) : (len == 1U))) {
		*((uint16_t *)((uint32_t)&base->FIFOWR) + 1) = (uint16_t)(tmpData >> 16U);
	} else {
		/* Clear the SPI_FIFOWR_EOT_MASK bit when data is not the last */
		tmpData &= (~(uint32_t)SPI_FIFOWR_EOT_MASK);
		*((uint16_t *)((uint32_t)&base->FIFOWR) + 1) = (uint16_t)(tmpData >> 16U);
	}

	/* gives the request ID */
	return dma_start(data->dma_tx.dma_dev, data->dma_tx.channel);
}

static int spi_mcux_dma_rx_load(const struct device *dev, uint8_t *buf,
				 size_t len)
{
	const struct spi_mcux_config *cfg = dev->config;
	struct spi_mcux_data *data = dev->data;
	struct dma_block_config *blk_cfg;
	int ret;
	SPI_Type *base = cfg->base;

	/* retrieve active RX DMA channel (used in callback) */
	struct stream *stream = &data->dma_rx;

	blk_cfg = &stream->dma_blk_cfg[0];

	/* prepare the block for this RX DMA channel */
	memset(blk_cfg, 0, sizeof(struct dma_block_config));
	blk_cfg->block_size = len;

	/* rx direction has periph as source and mem as dest. */
	if (buf == NULL) {
		/* if rx buff is null, then write data to dummy address. */
		blk_cfg->dest_address = (uint32_t)&dummy_rx_buffer;
		blk_cfg->dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	} else {
		blk_cfg->dest_address = (uint32_t)buf;
	}

	blk_cfg->source_address = (uint32_t)&base->FIFORD;

	/* direction is given by the DT */
	stream->dma_cfg.head_block = blk_cfg;
	stream->dma_cfg.user_data = (struct device *)dev;

	/* Enables the DMA request from SPI rxFIFO */
	base->FIFOCFG |= SPI_FIFOCFG_DMARX_MASK;

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

static int spi_mcux_dma_move_buffers(const struct device *dev, size_t len,
			const struct spi_config *spi_cfg, bool last_packet)
{
	struct spi_mcux_data *data = dev->data;
	int ret;

	ret = spi_mcux_dma_rx_load(dev, data->ctx.rx_buf, len);

	if (ret != 0) {
		return ret;
	}

	ret = spi_mcux_dma_tx_load(dev, data->ctx.tx_buf, spi_cfg,
							len, last_packet);

	return ret;
}

static int wait_dma_rx_tx_done(const struct device *dev)
{
	struct spi_mcux_data *data = dev->data;
	int ret = -1;

	while (1) {
		ret = spi_context_wait_for_completion(&data->ctx);
		if (data->status_flags & SPI_MCUX_FLEXCOMM_DMA_ERROR_FLAG) {
			return -EIO;
		}

		if ((data->status_flags & SPI_MCUX_FLEXCOMM_DMA_DONE_FLAG) ==
			SPI_MCUX_FLEXCOMM_DMA_DONE_FLAG) {
			return 0;
		}
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
	uint32_t word_size;
	uint16_t data_size;

	spi_context_lock(&data->ctx, asynchronous, cb, userdata, spi_cfg);

	ret = spi_mcux_configure(dev, spi_cfg);
	if (ret) {
		goto out;
	}

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

	spi_context_cs_control(&data->ctx, true);

	word_size = SPI_WORD_SIZE_GET(spi_cfg->operation);

	data_size = (word_size > 8) ? (sizeof(uint16_t)) : (sizeof(uint8_t));
	data->dma_rx.dma_cfg.source_data_size = data_size;
	data->dma_rx.dma_cfg.dest_data_size = data_size;
	data->dma_tx.dma_cfg.source_data_size = data_size;
	data->dma_tx.dma_cfg.dest_data_size = data_size;

	while (data->ctx.rx_len > 0 || data->ctx.tx_len > 0) {
		size_t dma_len;

		/* last is used to deassert chip select if this
		 * is the last transfer in the set.
		 */
		bool last = false;

		if (data->ctx.rx_len == 0) {
			dma_len = data->ctx.tx_len;
			last = true;
		} else if (data->ctx.tx_len == 0) {
			dma_len = data->ctx.rx_len;
			last = true;
		} else if (data->ctx.tx_len == data->ctx.rx_len) {
			dma_len = data->ctx.rx_len;
			last = true;
		} else {
			dma_len = MIN(data->ctx.tx_len, data->ctx.rx_len);
			last = false;
		}

		/* at this point, last just means whether or not
		 * this transfer will completely cover
		 * the current tx/rx buffer in data->ctx
		 * or require additional transfers because the
		 * the two buffers are not the same size.
		 *
		 * if it covers the current ctx tx/rx buffers, then
		 * we'll move to the next pair of buffers (if any)
		 * after the transfer, but if there are
		 * no more buffer pairs, then this is the last
		 * transfer in the set and we need to deassert CS.
		 */
		if (last) {
			/* this dma transfer should cover
			 * the entire current data->ctx set
			 * of buffers. if there are more
			 * buffers in the set, then we don't
			 * want to deassert CS.
			 */
			if ((data->ctx.tx_count > 1) ||
			    (data->ctx.rx_count > 1)) {
				/* more buffers to transfer so
				 * this isn't last
				 */
				last = false;
			}
		}

		data->status_flags = 0;

		ret = spi_mcux_dma_move_buffers(dev, dma_len, spi_cfg, last);
		if (ret != 0) {
			break;
		}

		ret = wait_dma_rx_tx_done(dev);
		if (ret != 0) {
			break;
		}

		/* wait until TX FIFO is really empty */
		while (0U == (base->FIFOSTAT & SPI_FIFOSTAT_TXEMPTY_MASK)) {
		}

		spi_context_update_tx(&data->ctx, 1, dma_len);
		spi_context_update_rx(&data->ctx, 1, dma_len);
	}

	base->FIFOCFG &= ~SPI_FIFOCFG_DMATX_MASK;
	base->FIFOCFG &= ~SPI_FIFOCFG_DMARX_MASK;

	spi_context_cs_control(&data->ctx, false);

out:
	spi_context_release(&data->ctx, ret);

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

	spi_context_lock(&data->ctx, asynchronous, cb, userdata, spi_cfg);

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

static int spi_mcux_init(const struct device *dev)
{
	int err;
	const struct spi_mcux_config *config = dev->config;
	struct spi_mcux_data *data = dev->data;

	config->irq_config_func(dev);

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

static const struct spi_driver_api spi_mcux_driver_api = {
	.transceive = spi_mcux_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_mcux_transceive_async,
#endif
	.release = spi_mcux_release,
};

#define SPI_MCUX_FLEXCOMM_IRQ_HANDLER_DECL(id)				\
	static void spi_mcux_config_func_##id(const struct device *dev)
#define SPI_MCUX_FLEXCOMM_IRQ_HANDLER_FUNC(id)				\
	.irq_config_func = spi_mcux_config_func_##id,
#define SPI_MCUX_FLEXCOMM_IRQ_HANDLER(id)				\
static void spi_mcux_config_func_##id(const struct device *dev) \
{								\
	IRQ_CONNECT(DT_INST_IRQN(id),				\
			DT_INST_IRQ(id, priority),			\
			spi_mcux_isr, DEVICE_DT_INST_GET(id),	\
			0);					\
	irq_enable(DT_INST_IRQN(id));				\
}

#ifndef CONFIG_SPI_MCUX_FLEXCOMM_DMA
#define SPI_DMA_CHANNELS(id)
#else
#define SPI_DMA_CHANNELS(id)				\
	.dma_tx = {						\
		.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(id, tx)), \
		.channel =					\
			DT_INST_DMAS_CELL_BY_NAME(id, tx, channel),	\
		.dma_cfg = {					\
			.channel_direction = MEMORY_TO_PERIPHERAL,	\
			.dma_callback = spi_mcux_dma_callback,		\
			.block_count = 2,		\
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
	};								\
	static struct spi_mcux_data spi_mcux_data_##id = {		\
		SPI_CONTEXT_INIT_LOCK(spi_mcux_data_##id, ctx),		\
		SPI_CONTEXT_INIT_SYNC(spi_mcux_data_##id, ctx),		\
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(id), ctx)	\
		SPI_DMA_CHANNELS(id)		\
	};								\
	DEVICE_DT_INST_DEFINE(id,					\
			    &spi_mcux_init,				\
			    NULL,					\
			    &spi_mcux_data_##id,			\
			    &spi_mcux_config_##id,			\
			    POST_KERNEL,				\
			    CONFIG_SPI_INIT_PRIORITY,			\
			    &spi_mcux_driver_api);			\
	\
	SPI_MCUX_FLEXCOMM_IRQ_HANDLER(id)

DT_INST_FOREACH_STATUS_OKAY(SPI_MCUX_FLEXCOMM_DEVICE)
