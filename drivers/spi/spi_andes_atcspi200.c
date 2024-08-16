/*
 * Copyright (c) 2022 Andes Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "spi_andes_atcspi200.h"

#include <zephyr/irq.h>

#define DT_DRV_COMPAT andestech_atcspi200

typedef void (*atcspi200_cfg_func_t)(void);

#ifdef CONFIG_ANDES_SPI_DMA_MODE

#define ANDES_SPI_DMA_ERROR_FLAG 0x01
#define ANDES_SPI_DMA_RX_DONE_FLAG 0x02
#define ANDES_SPI_DMA_TX_DONE_FLAG 0x04
#define ANDES_SPI_DMA_DONE_FLAG	\
	(ANDES_SPI_DMA_RX_DONE_FLAG | ANDES_SPI_DMA_TX_DONE_FLAG)

struct stream {
	const struct device *dma_dev;
	uint32_t channel;
	uint32_t block_idx;
	struct dma_config dma_cfg;
	struct dma_block_config dma_blk_cfg;
	struct dma_block_config chain_block[MAX_CHAIN_SIZE];
	uint8_t priority;
	bool src_addr_increment;
	bool dst_addr_increment;
};
#endif

struct spi_atcspi200_data {
	struct spi_context ctx;
	uint32_t tx_fifo_size;
	uint32_t rx_fifo_size;
	int tx_cnt;
	size_t chunk_len;
	bool busy;
#ifdef CONFIG_ANDES_SPI_DMA_MODE
	struct stream dma_rx;
	struct stream dma_tx;
#endif
};

struct spi_atcspi200_cfg {
	atcspi200_cfg_func_t cfg_func;
	uint32_t base;
	uint32_t irq_num;
	uint32_t f_sys;
	bool xip;
};

/* API Functions */
static int spi_config(const struct device *dev,
		      const struct spi_config *config)
{
	const struct spi_atcspi200_cfg * const cfg = dev->config;
	uint32_t sclk_div, data_len;

	/* Set the divisor for SPI interface sclk */
	sclk_div = (cfg->f_sys / (config->frequency << 1)) - 1;
	sys_clear_bits(SPI_TIMIN(cfg->base), TIMIN_SCLK_DIV_MSK);
	sys_set_bits(SPI_TIMIN(cfg->base), sclk_div);

	/* Set Master mode */
	sys_clear_bits(SPI_TFMAT(cfg->base), TFMAT_SLVMODE_MSK);

	/* Disable data merge mode */
	sys_clear_bits(SPI_TFMAT(cfg->base), TFMAT_DATA_MERGE_MSK);

	/* Set data length */
	data_len = SPI_WORD_SIZE_GET(config->operation) - 1;
	sys_clear_bits(SPI_TFMAT(cfg->base), TFMAT_DATA_LEN_MSK);
	sys_set_bits(SPI_TFMAT(cfg->base), (data_len << TFMAT_DATA_LEN_OFFSET));

	/* Set SPI frame format */
	if (config->operation & SPI_MODE_CPHA) {
		sys_set_bits(SPI_TFMAT(cfg->base), TFMAT_CPHA_MSK);
	} else {
		sys_clear_bits(SPI_TFMAT(cfg->base), TFMAT_CPHA_MSK);
	}

	if (config->operation & SPI_MODE_CPOL) {
		sys_set_bits(SPI_TFMAT(cfg->base), TFMAT_CPOL_MSK);
	} else {
		sys_clear_bits(SPI_TFMAT(cfg->base), TFMAT_CPOL_MSK);
	}

	/* Set SPI bit order */
	if (config->operation & SPI_TRANSFER_LSB) {
		sys_set_bits(SPI_TFMAT(cfg->base), TFMAT_LSB_MSK);
	} else {
		sys_clear_bits(SPI_TFMAT(cfg->base), TFMAT_LSB_MSK);
	}

	/* Set TX/RX FIFO threshold */
	sys_clear_bits(SPI_CTRL(cfg->base), CTRL_TX_THRES_MSK);
	sys_clear_bits(SPI_CTRL(cfg->base), CTRL_RX_THRES_MSK);

	sys_set_bits(SPI_CTRL(cfg->base), TX_FIFO_THRESHOLD << CTRL_TX_THRES_OFFSET);
	sys_set_bits(SPI_CTRL(cfg->base), RX_FIFO_THRESHOLD << CTRL_RX_THRES_OFFSET);

	return 0;
}

static int spi_transfer(const struct device *dev)
{
	struct spi_atcspi200_data * const data = dev->data;
	const struct spi_atcspi200_cfg * const cfg = dev->config;
	struct spi_context *ctx = &data->ctx;
	uint32_t data_len, tctrl, int_msk;

	if (data->chunk_len != 0) {
		data_len = data->chunk_len - 1;
	} else {
		data_len = 0;
	}

	if (data_len > MAX_TRANSFER_CNT) {
		return -EINVAL;
	}

	data->tx_cnt = 0;

	if (!spi_context_rx_on(ctx)) {
		tctrl = (TRNS_MODE_WRITE_ONLY << TCTRL_TRNS_MODE_OFFSET) |
			(data_len << TCTRL_WR_TCNT_OFFSET);
		int_msk = IEN_TX_FIFO_MSK | IEN_END_MSK;
	} else if (!spi_context_tx_on(ctx)) {
		tctrl = (TRNS_MODE_READ_ONLY << TCTRL_TRNS_MODE_OFFSET) |
			(data_len << TCTRL_RD_TCNT_OFFSET);
		int_msk = IEN_RX_FIFO_MSK | IEN_END_MSK;
	} else {
		tctrl = (TRNS_MODE_WRITE_READ << TCTRL_TRNS_MODE_OFFSET) |
			(data_len << TCTRL_WR_TCNT_OFFSET) |
			(data_len << TCTRL_RD_TCNT_OFFSET);
		int_msk = IEN_TX_FIFO_MSK |
			  IEN_RX_FIFO_MSK |
			  IEN_END_MSK;
	}

	sys_write32(tctrl, SPI_TCTRL(cfg->base));

	/* Enable TX/RX FIFO interrupts */
	sys_write32(int_msk, SPI_INTEN(cfg->base));

	/* Start transferring */
	sys_write32(0, SPI_CMD(cfg->base));

	return 0;
}


static int configure(const struct device *dev,
		     const struct spi_config *config)
{
	struct spi_atcspi200_data * const data = dev->data;
	struct spi_context *ctx = &(data->ctx);

	if (spi_context_configured(ctx, config)) {
		/* Already configured. No need to do it again. */
		return 0;
	}

	if (SPI_OP_MODE_GET(config->operation) != SPI_OP_MODE_MASTER) {
		LOG_ERR("Slave mode is not supported on %s",
			    dev->name);
		return -EINVAL;
	}

	if (config->operation & SPI_MODE_LOOP) {
		LOG_ERR("Loopback mode is not supported");
		return -EINVAL;
	}

	if ((config->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
		LOG_ERR("Only single line mode is supported");
		return -EINVAL;
	}

	ctx->config = config;

	/* SPI configuration */
	spi_config(dev, config);

	return 0;
}


#ifdef CONFIG_ANDES_SPI_DMA_MODE

static int spi_dma_tx_load(const struct device *dev);
static int spi_dma_rx_load(const struct device *dev);

static inline void spi_tx_dma_enable(const struct device *dev)
{
	const struct spi_atcspi200_cfg * const cfg = dev->config;
	/* Enable TX DMA */
	sys_set_bits(SPI_CTRL(cfg->base), CTRL_TX_DMA_EN_MSK);
}

static inline void spi_tx_dma_disable(const struct device *dev)
{
	const struct spi_atcspi200_cfg * const cfg = dev->config;
	/* Disable TX DMA */
	sys_clear_bits(SPI_CTRL(cfg->base), CTRL_TX_DMA_EN_MSK);
}

static inline void spi_rx_dma_enable(const struct device *dev)
{
	const struct spi_atcspi200_cfg * const cfg = dev->config;
	/* Enable RX DMA */
	sys_set_bits(SPI_CTRL(cfg->base), CTRL_RX_DMA_EN_MSK);
}

static inline void spi_rx_dma_disable(const struct device *dev)
{
	const struct spi_atcspi200_cfg * const cfg = dev->config;
	/* Disable RX DMA */
	sys_clear_bits(SPI_CTRL(cfg->base), CTRL_RX_DMA_EN_MSK);
}

static int spi_dma_move_buffers(const struct device *dev)
{
	struct spi_atcspi200_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t error = 0;

	data->dma_rx.dma_blk_cfg.next_block = NULL;
	data->dma_tx.dma_blk_cfg.next_block = NULL;

	if (spi_context_tx_on(ctx)) {
		error = spi_dma_tx_load(dev);
		if (error != 0) {
			return error;
		}
	}

	if (spi_context_rx_on(ctx)) {
		error = spi_dma_rx_load(dev);
		if (error != 0) {
			return error;
		}
	}

	return 0;
}

static inline void dma_rx_callback(const struct device *dev, void *user_data,
				uint32_t channel, int status)
{
	const struct device *spi_dev = (struct device *)user_data;
	struct spi_atcspi200_data *data = spi_dev->data;
	struct spi_context *ctx = &data->ctx;
	int error;

	dma_stop(data->dma_rx.dma_dev, data->dma_rx.channel);
	spi_rx_dma_disable(spi_dev);

	if (spi_context_rx_on(ctx)) {
		if (spi_dma_rx_load(spi_dev) != 0) {
			return;
		}
		spi_rx_dma_enable(spi_dev);
		error = dma_start(data->dma_rx.dma_dev, data->dma_rx.channel);
		__ASSERT(error == 0, "dma_start was failed in rx callback");
	}
}

static inline void dma_tx_callback(const struct device *dev, void *user_data,
				uint32_t channel, int status)
{
	const struct device *spi_dev = (struct device *)user_data;
	struct spi_atcspi200_data *data = spi_dev->data;
	struct spi_context *ctx = &data->ctx;
	int error;

	dma_stop(data->dma_tx.dma_dev, data->dma_tx.channel);
	spi_tx_dma_disable(spi_dev);

	if (spi_context_tx_on(ctx)) {
		if (spi_dma_tx_load(spi_dev) != 0) {
			return;
		}
		spi_tx_dma_enable(spi_dev);
		error = dma_start(data->dma_tx.dma_dev, data->dma_tx.channel);
		__ASSERT(error == 0, "dma_start was failed in tx callback");
	}
}

/*
 * dummy value used for transferring NOP when tx buf is null
 * and use as dummy sink for when rx buf is null
 */
uint32_t dummy_rx_tx_buffer;

static int spi_dma_tx_load(const struct device *dev)
{
	const struct spi_atcspi200_cfg * const cfg = dev->config;
	struct spi_atcspi200_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int remain_len, ret, dfs;

	/* prepare the block for this TX DMA channel */
	memset(&data->dma_tx.dma_blk_cfg, 0, sizeof(struct dma_block_config));

	if (ctx->current_tx->len > data->chunk_len) {
		data->dma_tx.dma_blk_cfg.block_size = data->chunk_len /
					data->dma_tx.dma_cfg.dest_data_size;
	} else {
		data->dma_tx.dma_blk_cfg.block_size = ctx->current_tx->len /
					data->dma_tx.dma_cfg.dest_data_size;
	}

	/* tx direction has memory as source and periph as dest. */
	if (ctx->current_tx->buf == NULL) {
		dummy_rx_tx_buffer = 0;
		/* if tx buff is null, then sends NOP on the line. */
		data->dma_tx.dma_blk_cfg.source_address = (uintptr_t)&dummy_rx_tx_buffer;
		data->dma_tx.dma_blk_cfg.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	} else {
		data->dma_tx.dma_blk_cfg.source_address = (uintptr_t)ctx->current_tx->buf;
		if (data->dma_tx.src_addr_increment) {
			data->dma_tx.dma_blk_cfg.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		} else {
			data->dma_tx.dma_blk_cfg.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		}
	}

	dfs = SPI_WORD_SIZE_GET(ctx->config->operation) >> 3;
	remain_len = data->chunk_len - ctx->current_tx->len;
	spi_context_update_tx(ctx, dfs, ctx->current_tx->len);

	data->dma_tx.dma_blk_cfg.dest_address = (uint32_t)SPI_DATA(cfg->base);
	/* fifo mode NOT USED there */
	if (data->dma_tx.dst_addr_increment) {
		data->dma_tx.dma_blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	} else {
		data->dma_tx.dma_blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	}

	/* direction is given by the DT */
	data->dma_tx.dma_cfg.head_block = &data->dma_tx.dma_blk_cfg;
	data->dma_tx.dma_cfg.head_block->next_block = NULL;
	/* give the client dev as arg, as the callback comes from the dma */
	data->dma_tx.dma_cfg.user_data = (void *)dev;

	if (data->dma_tx.dma_cfg.source_chaining_en) {
		data->dma_tx.dma_cfg.block_count = ctx->tx_count;
		data->dma_tx.dma_cfg.dma_callback = NULL;
		data->dma_tx.block_idx = 0;
		struct dma_block_config *blk_cfg = &data->dma_tx.dma_blk_cfg;
		const struct spi_buf *current_tx = ctx->current_tx;

		while (remain_len > 0) {
			struct dma_block_config *next_blk_cfg;

			next_blk_cfg = &data->dma_tx.chain_block[data->dma_tx.block_idx];
			data->dma_tx.block_idx += 1;

			blk_cfg->next_block = next_blk_cfg;
			current_tx = ctx->current_tx;

			next_blk_cfg->block_size = current_tx->len /
						data->dma_tx.dma_cfg.dest_data_size;

			/* tx direction has memory as source and periph as dest. */
			if (current_tx->buf == NULL) {
				dummy_rx_tx_buffer = 0;
				/* if tx buff is null, then sends NOP on the line. */
				next_blk_cfg->source_address = (uintptr_t)&dummy_rx_tx_buffer;
				next_blk_cfg->source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
			} else {
				next_blk_cfg->source_address = (uintptr_t)current_tx->buf;
				if (data->dma_tx.src_addr_increment) {
					next_blk_cfg->source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
				} else {
					next_blk_cfg->source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
				}
			}

			next_blk_cfg->dest_address = (uint32_t)SPI_DATA(cfg->base);
			/* fifo mode NOT USED there */
			if (data->dma_tx.dst_addr_increment) {
				next_blk_cfg->dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
			} else {
				next_blk_cfg->dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
			}

			blk_cfg = next_blk_cfg;
			next_blk_cfg->next_block = NULL;
			remain_len -= ctx->current_tx->len;
			spi_context_update_tx(ctx, dfs, ctx->current_tx->len);
		}

	} else {
		data->dma_tx.dma_blk_cfg.next_block = NULL;
		data->dma_tx.dma_cfg.block_count = 1;
		data->dma_tx.dma_cfg.dma_callback = dma_tx_callback;
	}

	/* pass our client origin to the dma: data->dma_tx.dma_channel */
	ret = dma_config(data->dma_tx.dma_dev, data->dma_tx.channel,
			&data->dma_tx.dma_cfg);
	/* the channel is the actual stream from 0 */
	if (ret != 0) {
		data->dma_tx.block_idx = 0;
		data->dma_tx.dma_blk_cfg.next_block = NULL;
		return ret;
	}

	return 0;
}

static int spi_dma_rx_load(const struct device *dev)
{
	const struct spi_atcspi200_cfg * const cfg = dev->config;
	struct spi_atcspi200_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int remain_len, ret, dfs;

	/* prepare the block for this RX DMA channel */
	memset(&data->dma_rx.dma_blk_cfg, 0, sizeof(struct dma_block_config));

	if (ctx->current_rx->len > data->chunk_len) {
		data->dma_rx.dma_blk_cfg.block_size = data->chunk_len /
					data->dma_rx.dma_cfg.dest_data_size;
	} else {
		data->dma_rx.dma_blk_cfg.block_size = ctx->current_rx->len /
					data->dma_rx.dma_cfg.dest_data_size;
	}

	/* rx direction has periph as source and mem as dest. */
	if (ctx->current_rx->buf == NULL) {
		/* if rx buff is null, then write data to dummy address. */
		data->dma_rx.dma_blk_cfg.dest_address = (uintptr_t)&dummy_rx_tx_buffer;
		data->dma_rx.dma_blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	} else {
		data->dma_rx.dma_blk_cfg.dest_address = (uintptr_t)ctx->current_rx->buf;
		if (data->dma_rx.dst_addr_increment) {
			data->dma_rx.dma_blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		} else {
			data->dma_rx.dma_blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		}
	}

	dfs = SPI_WORD_SIZE_GET(ctx->config->operation) >> 3;
	remain_len = data->chunk_len - ctx->current_rx->len;
	spi_context_update_rx(ctx, dfs, ctx->current_rx->len);

	data->dma_rx.dma_blk_cfg.source_address = (uint32_t)SPI_DATA(cfg->base);

	if (data->dma_rx.src_addr_increment) {
		data->dma_rx.dma_blk_cfg.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	} else {
		data->dma_rx.dma_blk_cfg.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	}

	data->dma_rx.dma_cfg.head_block = &data->dma_rx.dma_blk_cfg;
	data->dma_rx.dma_cfg.head_block->next_block = NULL;
	data->dma_rx.dma_cfg.user_data = (void *)dev;

	if (data->dma_rx.dma_cfg.source_chaining_en) {
		data->dma_rx.dma_cfg.block_count = ctx->rx_count;
		data->dma_rx.dma_cfg.dma_callback = NULL;
		data->dma_rx.block_idx = 0;
		struct dma_block_config *blk_cfg = &data->dma_rx.dma_blk_cfg;
		const struct spi_buf *current_rx = ctx->current_rx;

		while (remain_len > 0) {
			struct dma_block_config *next_blk_cfg;

			next_blk_cfg = &data->dma_rx.chain_block[data->dma_rx.block_idx];
			data->dma_rx.block_idx += 1;

			blk_cfg->next_block = next_blk_cfg;
			current_rx = ctx->current_rx;

			next_blk_cfg->block_size = current_rx->len /
						data->dma_rx.dma_cfg.dest_data_size;

			/* rx direction has periph as source and mem as dest. */
			if (current_rx->buf == NULL) {
				/* if rx buff is null, then write data to dummy address. */
				next_blk_cfg->dest_address = (uintptr_t)&dummy_rx_tx_buffer;
				next_blk_cfg->dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
			} else {
				next_blk_cfg->dest_address = (uintptr_t)current_rx->buf;
				if (data->dma_rx.dst_addr_increment) {
					next_blk_cfg->dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
				} else {
					next_blk_cfg->dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
				}
			}

			next_blk_cfg->source_address = (uint32_t)SPI_DATA(cfg->base);

			if (data->dma_rx.src_addr_increment) {
				next_blk_cfg->source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
			} else {
				next_blk_cfg->source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
			}

			blk_cfg = next_blk_cfg;
			next_blk_cfg->next_block = NULL;
			remain_len -= ctx->current_rx->len;
			spi_context_update_rx(ctx, dfs, ctx->current_rx->len);
		}
	} else {
		data->dma_rx.dma_blk_cfg.next_block = NULL;
		data->dma_rx.dma_cfg.block_count = 1;
		data->dma_rx.dma_cfg.dma_callback = dma_rx_callback;
	}

	/* pass our client origin to the dma: data->dma_rx.channel */
	ret = dma_config(data->dma_rx.dma_dev, data->dma_rx.channel,
			&data->dma_rx.dma_cfg);
	/* the channel is the actual stream from 0 */
	if (ret != 0) {
		data->dma_rx.block_idx = 0;
		data->dma_rx.dma_blk_cfg.next_block = NULL;
		return ret;
	}

	return 0;
}

static int spi_transfer_dma(const struct device *dev)
{
	const struct spi_atcspi200_cfg * const cfg = dev->config;
	struct spi_atcspi200_data * const data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t data_len, tctrl, dma_rx_enable, dma_tx_enable;
	int error = 0;

	data_len = data->chunk_len - 1;
	if (data_len > MAX_TRANSFER_CNT) {
		return -EINVAL;
	}

	if (!spi_context_rx_on(ctx)) {
		tctrl = (TRNS_MODE_WRITE_ONLY << TCTRL_TRNS_MODE_OFFSET) |
			(data_len << TCTRL_WR_TCNT_OFFSET);
		dma_rx_enable = 0;
		dma_tx_enable = 1;
	} else if (!spi_context_tx_on(ctx)) {
		tctrl = (TRNS_MODE_READ_ONLY << TCTRL_TRNS_MODE_OFFSET) |
			(data_len << TCTRL_RD_TCNT_OFFSET);
		dma_rx_enable = 1;
		dma_tx_enable = 0;
	} else {
		tctrl = (TRNS_MODE_WRITE_READ << TCTRL_TRNS_MODE_OFFSET) |
			(data_len << TCTRL_WR_TCNT_OFFSET) |
			(data_len << TCTRL_RD_TCNT_OFFSET);
		dma_rx_enable = 1;
		dma_tx_enable = 1;
	}

	sys_write32(tctrl, SPI_TCTRL(cfg->base));

	/* Set sclk_div to zero */
	sys_clear_bits(SPI_TIMIN(cfg->base), 0xff);

	/* Enable END Interrupts */
	sys_write32(IEN_END_MSK, SPI_INTEN(cfg->base));

	/* Setting DMA config*/
	error = spi_dma_move_buffers(dev);
	if (error != 0) {
		return error;
	}

	/* Start transferring */
	sys_write32(0, SPI_CMD(cfg->base));

	if (dma_rx_enable) {
		spi_rx_dma_enable(dev);
		error = dma_start(data->dma_rx.dma_dev, data->dma_rx.channel);
		if (error != 0) {
			return error;
		}
	}
	if (dma_tx_enable) {
		spi_tx_dma_enable(dev);
		error = dma_start(data->dma_tx.dma_dev, data->dma_tx.channel);
		if (error != 0) {
			return error;
		}
	}

	return 0;
}
#endif

static int transceive(const struct device *dev,
			  const struct spi_config *config,
			  const struct spi_buf_set *tx_bufs,
			  const struct spi_buf_set *rx_bufs,
			  bool asynchronous,
			  spi_callback_t cb,
			  void *userdata)
{
	const struct spi_atcspi200_cfg * const cfg = dev->config;
	struct spi_atcspi200_data * const data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int error, dfs;
	size_t chunk_len;

	spi_context_lock(ctx, asynchronous, cb, userdata, config);
	error = configure(dev, config);
	if (error == 0) {
		data->busy = true;

		dfs = SPI_WORD_SIZE_GET(ctx->config->operation) >> 3;
		spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, dfs);
		spi_context_cs_control(ctx, true);

		sys_set_bits(SPI_CTRL(cfg->base), CTRL_TX_FIFO_RST_MSK);
		sys_set_bits(SPI_CTRL(cfg->base), CTRL_RX_FIFO_RST_MSK);

		if (!spi_context_rx_on(ctx)) {
			chunk_len = spi_context_total_tx_len(ctx);
		} else if (!spi_context_tx_on(ctx)) {
			chunk_len = spi_context_total_rx_len(ctx);
		} else {
			size_t rx_len = spi_context_total_rx_len(ctx);
			size_t tx_len = spi_context_total_tx_len(ctx);

			chunk_len = MIN(rx_len, tx_len);
		}

		data->chunk_len = chunk_len;

#ifdef CONFIG_ANDES_SPI_DMA_MODE
		if ((data->dma_tx.dma_dev != NULL) && (data->dma_rx.dma_dev != NULL)) {
			error = spi_transfer_dma(dev);
			if (error != 0) {
				spi_context_cs_control(ctx, false);
				goto out;
			}
		} else {
#endif /* CONFIG_ANDES_SPI_DMA_MODE */

			error = spi_transfer(dev);
			if (error != 0) {
				spi_context_cs_control(ctx, false);
				goto out;
			}

#ifdef CONFIG_ANDES_SPI_DMA_MODE
		}
#endif /* CONFIG_ANDES_SPI_DMA_MODE */
		error = spi_context_wait_for_completion(ctx);
		spi_context_cs_control(ctx, false);
	}
out:
	spi_context_release(ctx, error);

	return error;
}

int spi_atcspi200_transceive(const struct device *dev,
			const struct spi_config *config,
			const struct spi_buf_set *tx_bufs,
			const struct spi_buf_set *rx_bufs)
{
	return transceive(dev, config, tx_bufs, rx_bufs, false, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
int spi_atcspi200_transceive_async(const struct device *dev,
				const struct spi_config *config,
				const struct spi_buf_set *tx_bufs,
				const struct spi_buf_set *rx_bufs,
				spi_callback_t cb,
				void *userdata)
{
	return transceive(dev, config, tx_bufs, rx_bufs, true, cb, userdata);
}
#endif

int spi_atcspi200_release(const struct device *dev,
			  const struct spi_config *config)
{

	struct spi_atcspi200_data * const data = dev->data;

	if (data->busy) {
		return -EBUSY;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

int spi_atcspi200_init(const struct device *dev)
{
	const struct spi_atcspi200_cfg * const cfg = dev->config;
	struct spi_atcspi200_data * const data = dev->data;
	int err = 0;

	/* we should not configure the device we are running on */
	if (cfg->xip) {
		return -EINVAL;
	}

	spi_context_unlock_unconditionally(&data->ctx);

#ifdef CONFIG_ANDES_SPI_DMA_MODE
	if (!data->dma_tx.dma_dev) {
		LOG_ERR("DMA device not found");
		return -ENODEV;
	}

	if (!data->dma_rx.dma_dev) {
		LOG_ERR("DMA device not found");
		return -ENODEV;
	}
#endif

	/* Get the TX/RX FIFO size of this device */
	data->tx_fifo_size = TX_FIFO_SIZE(cfg->base);
	data->rx_fifo_size = RX_FIFO_SIZE(cfg->base);

	cfg->cfg_func();

	irq_enable(cfg->irq_num);

	err = spi_context_cs_configure_all(&data->ctx);
	if (err < 0) {
		return err;
	}

	return 0;
}

static const struct spi_driver_api spi_atcspi200_api = {
	.transceive = spi_atcspi200_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_atcspi200_transceive_async,
#endif
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
	.release = spi_atcspi200_release
};

static void spi_atcspi200_irq_handler(void *arg)
{
	const struct device * const dev = (const struct device *) arg;
	const struct spi_atcspi200_cfg * const cfg = dev->config;
	struct spi_atcspi200_data * const data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t rx_data, cur_tx_fifo_num, cur_rx_fifo_num;
	uint32_t i, dfs, intr_status, spi_status;
	uint32_t tx_num = 0, tx_data = 0;
	int error = 0;

	intr_status = sys_read32(SPI_INTST(cfg->base));
	dfs = SPI_WORD_SIZE_GET(ctx->config->operation) >> 3;

	if ((intr_status & INTST_TX_FIFO_INT_MSK) &&
	    !(intr_status & INTST_END_INT_MSK)) {

		spi_status = sys_read32(SPI_STAT(cfg->base));
		cur_tx_fifo_num = GET_TX_NUM(cfg->base);

		tx_num = data->tx_fifo_size - cur_tx_fifo_num;

		for (i = tx_num; i > 0; i--) {

			if (data->tx_cnt >= data->chunk_len) {
				/* Have already sent a chunk of data, so stop
				 * sending data!
				 */
				sys_clear_bits(SPI_INTEN(cfg->base), IEN_TX_FIFO_MSK);
				break;
			}

			if (spi_context_tx_buf_on(ctx)) {

				switch (dfs) {
				case 1:
					tx_data = *ctx->tx_buf;
					break;
				case 2:
					tx_data = *(uint16_t *)ctx->tx_buf;
					break;
				}

			} else if (spi_context_tx_on(ctx)) {
				tx_data = 0;
			} else {
				sys_clear_bits(SPI_INTEN(cfg->base), IEN_TX_FIFO_MSK);
				break;
			}

			sys_write32(tx_data, SPI_DATA(cfg->base));

			spi_context_update_tx(ctx, dfs, 1);

			data->tx_cnt++;
		}
		sys_write32(INTST_TX_FIFO_INT_MSK, SPI_INTST(cfg->base));

	}

	if (intr_status & INTST_RX_FIFO_INT_MSK) {
		cur_rx_fifo_num = GET_RX_NUM(cfg->base);

		for (i = cur_rx_fifo_num; i > 0; i--) {

			rx_data = sys_read32(SPI_DATA(cfg->base));

			if (spi_context_rx_buf_on(ctx)) {

				switch (dfs) {
				case 1:
					*ctx->rx_buf = rx_data;
					break;
				case 2:
					*(uint16_t *)ctx->rx_buf = rx_data;
					break;
				}

			} else if (!spi_context_rx_on(ctx)) {
				sys_clear_bits(SPI_INTEN(cfg->base), IEN_RX_FIFO_MSK);
			}

			spi_context_update_rx(ctx, dfs, 1);
		}
		sys_write32(INTST_RX_FIFO_INT_MSK, SPI_INTST(cfg->base));
	}

	if (intr_status & INTST_END_INT_MSK) {

		/* Clear end interrupt */
		sys_write32(INTST_END_INT_MSK, SPI_INTST(cfg->base));

		/* Disable all SPI interrupts */
		sys_write32(0, SPI_INTEN(cfg->base));

#ifdef CONFIG_ANDES_SPI_DMA_MODE
		if ((data->dma_tx.dma_dev != NULL) && data->dma_tx.dma_cfg.source_chaining_en) {

			spi_tx_dma_disable(dev);
			dma_stop(data->dma_tx.dma_dev, data->dma_tx.channel);
			data->dma_tx.block_idx = 0;
			data->dma_tx.dma_blk_cfg.next_block = NULL;
		}

		if ((data->dma_rx.dma_dev != NULL) && data->dma_rx.dma_cfg.source_chaining_en) {

			spi_rx_dma_disable(dev);
			dma_stop(data->dma_rx.dma_dev, data->dma_rx.channel);
			data->dma_rx.block_idx = 0;
			data->dma_rx.dma_blk_cfg.next_block = NULL;
		}
#endif /* CONFIG_ANDES_SPI_DMA_MODE */

		data->busy = false;

		spi_context_complete(ctx, dev, error);

	}
}

#if CONFIG_ANDES_SPI_DMA_MODE

#define ANDES_DMA_CONFIG_DIRECTION(config)		(FIELD_GET(GENMASK(1, 0), config))
#define ANDES_DMA_CONFIG_PERIPHERAL_ADDR_INC(config)	(FIELD_GET(BIT(2), config))
#define ANDES_DMA_CONFIG_MEMORY_ADDR_INC(config)	(FIELD_GET(BIT(3), config))
#define ANDES_DMA_CONFIG_PERIPHERAL_DATA_SIZE(config)	(1 << (FIELD_GET(GENMASK(6, 4), config)))
#define ANDES_DMA_CONFIG_MEMORY_DATA_SIZE(config)	(1 << (FIELD_GET(GENMASK(9, 7), config)))
#define ANDES_DMA_CONFIG_PRIORITY(config)		(FIELD_GET(BIT(10), config))

#define DMA_CHANNEL_CONFIG(id, dir)						\
		DT_INST_DMAS_CELL_BY_NAME(id, dir, channel_config)

#define SPI_DMA_CHANNEL_INIT(index, dir, dir_cap, src_dev, dest_dev)		\
	.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(index, dir)),	\
	.channel =								\
		DT_INST_DMAS_CELL_BY_NAME(index, dir, channel),			\
	.dma_cfg = {								\
		.dma_slot =							\
		   DT_INST_DMAS_CELL_BY_NAME(index, dir, slot),			\
		.channel_direction = ANDES_DMA_CONFIG_DIRECTION(		\
				     DMA_CHANNEL_CONFIG(index, dir)),		\
		.complete_callback_en = 0,					\
		.error_callback_dis = 0,					\
		.source_data_size =						\
			ANDES_DMA_CONFIG_##src_dev##_DATA_SIZE(			\
					DMA_CHANNEL_CONFIG(index, dir)		\
					),					\
		.dest_data_size =						\
			ANDES_DMA_CONFIG_##dest_dev##_DATA_SIZE(		\
					DMA_CHANNEL_CONFIG(index, dir)		\
					),					\
		.source_burst_length = 1, /* SINGLE transfer */			\
		.dest_burst_length = 1, /* SINGLE transfer */			\
		.channel_priority = ANDES_DMA_CONFIG_PRIORITY(			\
					DMA_CHANNEL_CONFIG(index, dir)		\
					),					\
		.source_chaining_en = DT_PROP(DT_INST_DMAS_CTLR_BY_NAME(	\
					index, dir), chain_transfer),		\
		.dest_chaining_en = DT_PROP(DT_INST_DMAS_CTLR_BY_NAME(		\
					index, dir), chain_transfer),		\
	},									\
	.src_addr_increment =							\
			ANDES_DMA_CONFIG_##src_dev##_ADDR_INC(			\
					DMA_CHANNEL_CONFIG(index, dir)		\
					),					\
	.dst_addr_increment =							\
			ANDES_DMA_CONFIG_##dest_dev##_ADDR_INC(			\
					DMA_CHANNEL_CONFIG(index, dir)		\
					)

#define SPI_DMA_CHANNEL(id, dir, DIR, src, dest)				\
	.dma_##dir = {								\
		COND_CODE_1(DT_INST_DMAS_HAS_NAME(id, dir),			\
			(SPI_DMA_CHANNEL_INIT(id, dir, DIR, src, dest)),	\
			(NULL))							\
		},

#else
#define SPI_DMA_CHANNEL(id, dir, DIR, src, dest)
#endif

#define SPI_BUSY_INIT .busy = false,

#if (CONFIG_XIP)
#define SPI_ROM_CFG_XIP(node_id) DT_SAME_NODE(node_id, DT_BUS(DT_CHOSEN(zephyr_flash)))
#else
#define SPI_ROM_CFG_XIP(node_id) false
#endif

#define SPI_INIT(n)							\
	static struct spi_atcspi200_data spi_atcspi200_dev_data_##n = { \
		SPI_CONTEXT_INIT_LOCK(spi_atcspi200_dev_data_##n, ctx),	\
		SPI_CONTEXT_INIT_SYNC(spi_atcspi200_dev_data_##n, ctx),	\
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)	\
		SPI_BUSY_INIT						\
		SPI_DMA_CHANNEL(n, rx, RX, PERIPHERAL, MEMORY)		\
		SPI_DMA_CHANNEL(n, tx, TX, MEMORY, PERIPHERAL)		\
	};								\
	static void spi_atcspi200_cfg_##n(void);			\
	static struct spi_atcspi200_cfg spi_atcspi200_dev_cfg_##n = {	\
		.cfg_func = spi_atcspi200_cfg_##n,			\
		.base = DT_INST_REG_ADDR(n),				\
		.irq_num = DT_INST_IRQN(n),				\
		.f_sys = DT_INST_PROP(n, clock_frequency),		\
		.xip = SPI_ROM_CFG_XIP(DT_DRV_INST(n)),			\
	};								\
									\
	DEVICE_DT_INST_DEFINE(n,					\
		spi_atcspi200_init,					\
		NULL,							\
		&spi_atcspi200_dev_data_##n,				\
		&spi_atcspi200_dev_cfg_##n,				\
		POST_KERNEL,						\
		CONFIG_SPI_INIT_PRIORITY,				\
		&spi_atcspi200_api);					\
									\
	static void spi_atcspi200_cfg_##n(void)				\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
				DT_INST_IRQ(n, priority),		\
				spi_atcspi200_irq_handler,		\
				DEVICE_DT_INST_GET(n),			\
				0);					\
	};

DT_INST_FOREACH_STATUS_OKAY(SPI_INIT)
