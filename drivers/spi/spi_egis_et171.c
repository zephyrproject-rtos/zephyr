/*
 * Copyright (c) 2022 Andes Technology Corporation.
 * Copyright (c) 2025 Egis Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "spi_egis_et171.h"

#include <zephyr/irq.h>

#define DT_DRV_COMPAT egis_et171_spi

typedef void (*et171_cfg_func_t)(void);
#ifdef CONFIG_DCACHE
#ifdef CONFIG_CACHE_MANAGEMENT
#include <zephyr/cache.h>
#define IS_ALIGN(x)          (((uintptr_t)(x) & (sys_cache_data_line_size_get() - 1)) == 0)
#define DRAM_START	     CONFIG_SRAM_BASE_ADDRESS
#define DRAM_SIZE	     KB(CONFIG_SRAM_SIZE)
#define DRAM_END	     (DRAM_START + DRAM_SIZE - 1)
#define IS_ADDR_IN_RAM(addr) (((addr) >= DRAM_START) && ((addr) <= DRAM_END))
#endif

struct revert_info {
	void *dst_buf;
	void *src_buf;
	size_t len;
};

struct dma_align_context {
	struct spi_buf *rx_bufs;
	size_t count;
	struct revert_info *revert_infos;
	void *align_buffer;
};
#endif

#ifdef CONFIG_EGIS_SPI_DMA_MODE

#define EGIS_SPI_DMA_ERROR_FLAG 0x01
#define EGIS_SPI_DMA_RX_DONE_FLAG 0x02
#define EGIS_SPI_DMA_TX_DONE_FLAG 0x04
#define EGIS_SPI_DMA_DONE_FLAG	\
	(EGIS_SPI_DMA_RX_DONE_FLAG | EGIS_SPI_DMA_TX_DONE_FLAG)

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

struct spi_et171_data {
	struct spi_context ctx;
	uint32_t tx_fifo_size;
	uint32_t rx_fifo_size;
	int tx_cnt;
	size_t chunk_len;
	bool busy;
#ifdef CONFIG_EGIS_SPI_DMA_MODE
	struct stream dma_rx;
	struct stream dma_tx;
#endif
#ifdef CONFIG_DCACHE
	struct dma_align_context dma_buf_ctx;
	struct spi_buf_set aligned_rx_bufs;
#endif
};

struct spi_et171_cfg {
	et171_cfg_func_t cfg_func;
	uint32_t base;
	uint32_t irq_num;
	uint32_t f_sys;
	bool xip;
};

static inline bool spi_transfer_needed(const struct spi_buf_set *tx_bufs,
				       const struct spi_buf_set *rx_bufs)
{
	size_t tlen = 0, rlen = 0;

	if (tx_bufs) {
		for (size_t i = 0; i < tx_bufs->count; i++) {
			tlen += tx_bufs->buffers[i].len;
		}
	}

	if (rx_bufs) {
		for (size_t i = 0; i < rx_bufs->count; i++) {
			rlen += rx_bufs->buffers[i].len;
		}
	}

	return (tlen > 0) || (rlen > 0);
}

/* API Functions */
static int spi_config(const struct device *dev,
		      const struct spi_config *config)
{
	const struct spi_et171_cfg * const cfg = dev->config;
	uint32_t sclk_div, data_len;

	/* Set the divisor for SPI interface sclk */
	sclk_div = (cfg->f_sys / 2 + config->frequency - 1) / config->frequency - 1;
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
	struct spi_et171_data * const data = dev->data;
	const struct spi_et171_cfg *const cfg = dev->config;
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
		tctrl = (TRNS_MODE_WRITE_ONLY << TCTRL_TRNS_MODE_OFFSET);
		int_msk = IEN_TX_FIFO_MSK | IEN_END_MSK;
		sys_write32(data_len, SPI_WR_TRANS_CNT(cfg->base));
	} else if (!spi_context_tx_on(ctx)) {
		tctrl = (TRNS_MODE_READ_ONLY << TCTRL_TRNS_MODE_OFFSET);
		int_msk = IEN_RX_FIFO_MSK | IEN_END_MSK;
		sys_write32(data_len, SPI_RD_TRANS_CNT(cfg->base));
	} else {
		tctrl = (TRNS_MODE_WRITE_READ << TCTRL_TRNS_MODE_OFFSET);
		int_msk = IEN_TX_FIFO_MSK | IEN_RX_FIFO_MSK | IEN_END_MSK;
		sys_write32(data_len, SPI_WR_TRANS_CNT(cfg->base));
		sys_write32(data_len, SPI_RD_TRANS_CNT(cfg->base));
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
	struct spi_et171_data * const data = dev->data;
	struct spi_context *ctx = &(data->ctx);

	if (spi_context_configured(ctx, config) && (ctx->config->frequency == config->frequency)) {
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

	if ((SPI_WORD_SIZE_GET(config->operation) != 8) &&
	    (SPI_WORD_SIZE_GET(config->operation) != 16)) {
		return -ENOTSUP;
	}

	ctx->config = config;

	/* SPI configuration */
	spi_config(dev, config);

	return 0;
}


#ifdef CONFIG_EGIS_SPI_DMA_MODE

static int spi_dma_tx_load(const struct device *dev);
static int spi_dma_rx_load(const struct device *dev);

static inline void spi_tx_dma_enable(const struct device *dev)
{
	const struct spi_et171_cfg *const cfg = dev->config;
	/* Enable TX DMA */
	sys_set_bits(SPI_CTRL(cfg->base), CTRL_TX_DMA_EN_MSK);
}

static inline void spi_tx_dma_disable(const struct device *dev)
{
	const struct spi_et171_cfg *const cfg = dev->config;
	/* Disable TX DMA */
	sys_clear_bits(SPI_CTRL(cfg->base), CTRL_TX_DMA_EN_MSK);
}

static inline void spi_rx_dma_enable(const struct device *dev)
{
	const struct spi_et171_cfg *const cfg = dev->config;
	/* Enable RX DMA */
	sys_set_bits(SPI_CTRL(cfg->base), CTRL_RX_DMA_EN_MSK);
}

static inline void spi_rx_dma_disable(const struct device *dev)
{
	const struct spi_et171_cfg *const cfg = dev->config;
	/* Disable RX DMA */
	sys_clear_bits(SPI_CTRL(cfg->base), CTRL_RX_DMA_EN_MSK);
}

static int spi_dma_move_buffers(const struct device *dev)
{
	struct spi_et171_data *data = dev->data;
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
	struct spi_et171_data *data = (struct spi_et171_data *)user_data;
	const struct device *spi_dev = CONTAINER_OF(user_data, const struct device, data);
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

		if (error != 0) {
			LOG_ERR("dma_start failed in RX callback (err: %d)", error);
			return;
		}
	}
}

static inline void dma_tx_callback(const struct device *dev, void *user_data,
				uint32_t channel, int status)
{

	struct spi_et171_data *data = (struct spi_et171_data *)user_data;
	const struct device *spi_dev = CONTAINER_OF(user_data, const struct device, data);
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

		if (error != 0) {
			LOG_ERR("dma_start failed in TX callback (err: %d)", error);
			return;
		}
	}
}

/*
 * dummy value used for transferring NOP when tx buf is null
 * and use as dummy sink for when rx buf is null
 */
uint32_t dummy_rxtx_buffer;

static void configure_tx_dma_block_source(struct dma_block_config *blk_cfg,
					  const struct spi_buf *tx_buf, struct spi_et171_data *data,
					  size_t len)
{
	if (tx_buf->buf == NULL) {
		dummy_rxtx_buffer = 0;
		blk_cfg->source_address = (uintptr_t)&dummy_rxtx_buffer;
		blk_cfg->source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	} else {
		blk_cfg->source_address = (uintptr_t)tx_buf->buf;

#ifdef CONFIG_DCACHE
#ifdef CONFIG_CACHE_MANAGEMENT
		if (IS_ADDR_IN_RAM(blk_cfg->source_address)) {
			cache_data_flush_range((void *)blk_cfg->source_address, len);
		}
#else
#error "With Cache enable, you may need to do the cache flush before reset."
#endif
#endif

		blk_cfg->source_addr_adj = data->dma_tx.src_addr_increment ? DMA_ADDR_ADJ_INCREMENT
									   : DMA_ADDR_ADJ_NO_CHANGE;
	}
}

static void configure_tx_dma_block_dest(struct dma_block_config *blk_cfg,
					struct spi_et171_data *data,
					const struct spi_et171_cfg *cfg)
{
	blk_cfg->dest_address = (uint32_t)SPI_DATA(cfg->base);
	blk_cfg->dest_addr_adj =
		data->dma_tx.dst_addr_increment ? DMA_ADDR_ADJ_INCREMENT : DMA_ADDR_ADJ_NO_CHANGE;
}

static int spi_dma_tx_load(const struct device *dev)
{
	const struct spi_et171_cfg *const cfg = dev->config;
	struct spi_et171_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int remain_len, ret;
	int dfs = SPI_WORD_SIZE_GET(ctx->config->operation) >> 3;

	struct dma_block_config *blk_cfg = &data->dma_tx.dma_blk_cfg;
	const struct spi_buf *current_tx = ctx->current_tx;

	/* prepare the block for this TX DMA channel */
	memset(&data->dma_tx.dma_blk_cfg, 0, sizeof(struct dma_block_config));

	data->dma_tx.dma_blk_cfg.block_size =
		MIN(ctx->current_tx->len, data->chunk_len) / data->dma_tx.dma_cfg.dest_data_size;

	/* tx direction has memory as source and periph as dest. */
	configure_tx_dma_block_source(blk_cfg, current_tx, data, current_tx->len);

	remain_len = data->chunk_len - ctx->current_tx->len;
	spi_context_update_tx(ctx, dfs, ctx->current_tx->len);
	configure_tx_dma_block_dest(blk_cfg, data, cfg);

	/* direction is given by the DT */
	data->dma_tx.dma_cfg.head_block = blk_cfg;
	data->dma_tx.dma_cfg.head_block->next_block = NULL;
	/* give the client dev as arg, as the callback comes from the dma */
	data->dma_tx.dma_cfg.user_data = (void *)data;

	if (data->dma_tx.dma_cfg.source_chaining_en) {
		data->dma_tx.dma_cfg.block_count = ctx->tx_count;
		data->dma_tx.dma_cfg.dma_callback = NULL;
		data->dma_tx.block_idx = 0;

		while (remain_len > 0) {
			struct dma_block_config *next_blk_cfg =
				&data->dma_tx.chain_block[data->dma_tx.block_idx];
			data->dma_tx.block_idx += 1;

			blk_cfg->next_block = next_blk_cfg;
			current_tx = ctx->current_tx;

			next_blk_cfg->block_size = current_tx->len /
						data->dma_tx.dma_cfg.dest_data_size;

			/* tx direction has memory as source and periph as dest. */
			configure_tx_dma_block_source(next_blk_cfg, current_tx, data,
						      current_tx->len);
			configure_tx_dma_block_dest(next_blk_cfg, data, cfg);

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

static void configure_rx_dma_block_dest(struct dma_block_config *blk_cfg,
					const struct spi_buf *rx_buf, struct spi_et171_data *data,
					size_t len)
{
	if (rx_buf->buf == NULL) {
		blk_cfg->dest_address = (uintptr_t)&dummy_rxtx_buffer;
		blk_cfg->dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	} else {
		blk_cfg->dest_address = (uintptr_t)rx_buf->buf;

#ifdef CONFIG_DCACHE
#ifdef CONFIG_CACHE_MANAGEMENT
		if (IS_ADDR_IN_RAM(blk_cfg->dest_address)) {
			cache_data_invd_range((void *)blk_cfg->dest_address, len);
		}
#else
#error "With Cache enable, you may need to do the cache flush before reset."
#endif
#endif

		blk_cfg->dest_addr_adj = data->dma_rx.dst_addr_increment ? DMA_ADDR_ADJ_INCREMENT
									 : DMA_ADDR_ADJ_NO_CHANGE;
	}
}

static void configure_rx_dma_block_source(struct dma_block_config *blk_cfg,
					  struct spi_et171_data *data,
					  const struct spi_et171_cfg *cfg)
{
	blk_cfg->source_address = (uint32_t)SPI_DATA(cfg->base);
	blk_cfg->source_addr_adj =
		data->dma_rx.src_addr_increment ? DMA_ADDR_ADJ_INCREMENT : DMA_ADDR_ADJ_NO_CHANGE;
}

static int spi_dma_rx_load(const struct device *dev)
{
	const struct spi_et171_cfg *const cfg = dev->config;
	struct spi_et171_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int remain_len, ret;
	int dfs = SPI_WORD_SIZE_GET(ctx->config->operation) >> 3;

	struct dma_block_config *blk_cfg = &data->dma_rx.dma_blk_cfg;
	const struct spi_buf *current_rx = ctx->current_rx;

	/* prepare the block for this RX DMA channel */
	memset(&data->dma_rx.dma_blk_cfg, 0, sizeof(struct dma_block_config));

	data->dma_rx.dma_blk_cfg.block_size =
		MIN(ctx->current_rx->len, data->chunk_len) / data->dma_rx.dma_cfg.dest_data_size;

	/* rx direction has periph as source and mem as dest. */
	configure_rx_dma_block_dest(blk_cfg, current_rx, data, current_rx->len);

	remain_len = data->chunk_len - ctx->current_rx->len;
	spi_context_update_rx(ctx, dfs, ctx->current_rx->len);
	configure_rx_dma_block_source(blk_cfg, data, cfg);

	data->dma_rx.dma_cfg.head_block = blk_cfg;
	data->dma_rx.dma_cfg.head_block->next_block = NULL;
	data->dma_rx.dma_cfg.user_data = (void *)data;

	if (data->dma_rx.dma_cfg.source_chaining_en) {
		data->dma_rx.dma_cfg.block_count = ctx->rx_count;
		data->dma_rx.dma_cfg.dma_callback = NULL;
		data->dma_rx.block_idx = 0;

		while (remain_len > 0) {
			struct dma_block_config *next_blk_cfg =
				&data->dma_rx.chain_block[data->dma_rx.block_idx];
			data->dma_rx.block_idx += 1;

			blk_cfg->next_block = next_blk_cfg;
			current_rx = ctx->current_rx;

			next_blk_cfg->block_size = current_rx->len /
						data->dma_rx.dma_cfg.dest_data_size;

			/* rx direction has periph as source and mem as dest. */
			configure_rx_dma_block_dest(next_blk_cfg, current_rx, data,
						    current_rx->len);
			configure_rx_dma_block_source(next_blk_cfg, data, cfg);

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
	const struct spi_et171_cfg *const cfg = dev->config;
	struct spi_et171_data * const data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t data_len, tctrl, dma_rx_enable, dma_tx_enable;
	int error = 0;

	data_len = data->chunk_len - 1;

	if (data_len > MAX_TRANSFER_CNT) {
		return -EINVAL;
	}

	if (!spi_context_rx_on(ctx)) {
		tctrl = (TRNS_MODE_WRITE_ONLY << TCTRL_TRNS_MODE_OFFSET);
		dma_rx_enable = 0;
		dma_tx_enable = 1;
		sys_write32(data_len, SPI_WR_TRANS_CNT(cfg->base));
	} else if (!spi_context_tx_on(ctx)) {
		tctrl = (TRNS_MODE_READ_ONLY << TCTRL_TRNS_MODE_OFFSET);
		dma_rx_enable = 1;
		dma_tx_enable = 0;
		sys_write32(data_len, SPI_RD_TRANS_CNT(cfg->base));
	} else {
		tctrl = (TRNS_MODE_WRITE_READ << TCTRL_TRNS_MODE_OFFSET);
		dma_rx_enable = 1;
		dma_tx_enable = 1;
		sys_write32(data_len, SPI_WR_TRANS_CNT(cfg->base));
		sys_write32(data_len, SPI_RD_TRANS_CNT(cfg->base));
	}

	sys_write32(tctrl, SPI_TCTRL(cfg->base));

	/* Setting DMA config*/
	error = spi_dma_move_buffers(dev);
	if (error != 0) {
		return error;
	}

	/* Enable END Interrupts */
	sys_write32(IEN_END_MSK, SPI_INTEN(cfg->base));
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

static int transceive(const struct device *dev, const struct spi_config *config,
		      const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs)
{
	const struct spi_et171_cfg *const cfg = dev->config;
	struct spi_et171_data * const data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int error, dfs;
	size_t chunk_len;

	error = configure(dev, config);
	if (error != 0) {
		goto out;
	}

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

#ifdef CONFIG_EGIS_SPI_DMA_MODE
	if ((data->dma_tx.dma_dev != NULL) && (data->dma_rx.dma_dev != NULL)) {
		error = spi_transfer_dma(dev);
		if (error != 0) {
			spi_context_cs_control(ctx, false);
			goto out;
		}
	} else {
#endif /* CONFIG_EGIS_SPI_DMA_MODE */

		error = spi_transfer(dev);
		if (error != 0) {
			spi_context_cs_control(ctx, false);
			goto out;
		}

#ifdef CONFIG_EGIS_SPI_DMA_MODE
	}
#endif /* CONFIG_EGIS_SPI_DMA_MODE */
	error = spi_context_wait_for_completion(ctx);
	spi_context_cs_control(ctx, false);
out:
	return error;
}

#ifdef CONFIG_DCACHE
int is_need_alignment(const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs,
		      int *extend_count, int *frag_size)
{
	int cache_line_size = sys_cache_data_line_size_get();
	*extend_count = 0;
	*frag_size = 0;

	if (!rx_bufs) {
		return false;
	}

	for (int i = 0; i < rx_bufs->count; i++) {
		const struct spi_buf *const rx_buf = &rx_bufs->buffers[i];

		if (rx_buf->buf == NULL) {
			continue;
		}
		if (!IS_ALIGN(rx_buf->buf)) {
			if (rx_buf->len <= (2 * cache_line_size)) {
				/* move all */
				*frag_size += rx_buf->len;
			} else {
				/* head */
				*frag_size += ROUND_UP((size_t)rx_buf->buf, cache_line_size) -
					      (size_t)rx_buf->buf;
				*extend_count += 1;
				/* tail */
				if (((size_t)rx_buf->buf + rx_buf->len) !=
				    ROUND_DOWN((size_t)rx_buf->buf + rx_buf->len,
					       cache_line_size)) {
					*frag_size += ((size_t)rx_buf->buf + rx_buf->len) -
						      ROUND_DOWN((size_t)rx_buf->buf + rx_buf->len,
								 cache_line_size);
					*extend_count += 1;
				}
			}
			continue;
		}

		if (!IS_ALIGN(rx_buf->len)) {
			if (rx_buf->len <= (2 * cache_line_size)) {
				/* move all */
				*frag_size += rx_buf->len;
			} else {
				/* tail */
				*frag_size += ((size_t)rx_buf->buf + rx_buf->len) -
					      ROUND_DOWN((size_t)rx_buf->buf + rx_buf->len,
							 cache_line_size);
				*extend_count += 1;
			}
		}
	}

	if (*frag_size) {
		return true;
	} else {
		return false;
	}
}

static int allocate_dma_buffers(struct dma_align_context *ctx, int buf_count, size_t align_buf_size,
				size_t cache_line_size)
{
	ctx->rx_bufs = k_malloc(sizeof(struct spi_buf) * buf_count);
	ctx->revert_infos = k_malloc(sizeof(struct revert_info) * buf_count);
	ctx->align_buffer = k_aligned_alloc(cache_line_size, align_buf_size);

	if (!ctx->rx_bufs || !ctx->revert_infos || !ctx->align_buffer) {
		printk("alloc memory fail\n");
		return -ENOMEM;
	}

	memset(ctx->rx_bufs, 0, sizeof(struct spi_buf) * buf_count);
	memset(ctx->revert_infos, 0, sizeof(struct revert_info) * buf_count);
	memset(ctx->align_buffer, 0xFF, align_buf_size);

	return 0;
}

static void free_dma_buffers(struct dma_align_context *ctx)
{
	ctx->count = 0;
	if (ctx->align_buffer) {
		k_free(ctx->align_buffer);
		ctx->align_buffer = NULL;
	}
	if (ctx->rx_bufs) {
		k_free(ctx->rx_bufs);
		ctx->rx_bufs = NULL;
	}
	if (ctx->revert_infos) {
		k_free(ctx->revert_infos);
		ctx->revert_infos = NULL;
	}
}

void revert_dma_buffers(struct revert_info *infos, int count)
{
	for (int i = 0; i < count; i++) {
		if (infos[i].len > 0) {
			memcpy(infos[i].dst_buf, infos[i].src_buf, infos[i].len);
		}
	}
}

void process_rx_buf(const struct spi_buf *rx_buf, uint8_t **p_buf, struct spi_buf **rx_buf_ptr,
		    struct revert_info **revert_ptr, size_t cache_line_size)
{
	if (!rx_buf->buf) {
		(*revert_ptr)->len = 0;
		**rx_buf_ptr = *rx_buf;

		(*rx_buf_ptr)++;
		(*revert_ptr)++;
		return;
	}

	uintptr_t buf_start = (uintptr_t)rx_buf->buf;
	uintptr_t buf_end = buf_start + rx_buf->len;
	uintptr_t aligned_start = ROUND_UP(buf_start, cache_line_size);
	uintptr_t aligned_end = ROUND_DOWN(buf_end, cache_line_size);

	if (!IS_ALIGN(rx_buf->buf)) {
		if (rx_buf->len <= (2 * cache_line_size)) {
			/* move all */
			(*revert_ptr)->dst_buf = rx_buf->buf;
			(*revert_ptr)->src_buf = (*rx_buf_ptr)->buf = *p_buf;
			(*rx_buf_ptr)->len = (*revert_ptr)->len = rx_buf->len;
			*p_buf += rx_buf->len;

			(*revert_ptr)++;
			(*rx_buf_ptr)++;

		} else {
			/* head */
			size_t head_len = aligned_start - buf_start;
			(*revert_ptr)->dst_buf = rx_buf->buf;
			(*revert_ptr)->src_buf = (*rx_buf_ptr)->buf = *p_buf;
			(*rx_buf_ptr)->len = (*revert_ptr)->len = head_len;
			*p_buf += head_len;

			(*rx_buf_ptr)++;
			(*revert_ptr)++;

			/* body */
			size_t body_len = aligned_end - aligned_start;
			(*revert_ptr)->len = 0;
			(*rx_buf_ptr)->buf = (void *)aligned_start;
			(*rx_buf_ptr)->len = body_len;

			(*rx_buf_ptr)++;
			(*revert_ptr)++;

			/* tail */
			if (aligned_end != buf_end) {
				size_t tail_len = buf_end - aligned_end;
				(*revert_ptr)->dst_buf = (void *)aligned_end;
				(*revert_ptr)->src_buf = (*rx_buf_ptr)->buf = *p_buf;
				(*rx_buf_ptr)->len = (*revert_ptr)->len = tail_len;
				*p_buf += tail_len;

				(*rx_buf_ptr)++;
				(*revert_ptr)++;
			}
			return;
		}
	} else if (!IS_ALIGN(rx_buf->len)) {
		if (rx_buf->len <= (2 * cache_line_size)) {
			(*revert_ptr)->dst_buf = rx_buf->buf;
			(*revert_ptr)->src_buf = (*rx_buf_ptr)->buf = *p_buf;
			(*rx_buf_ptr)->len = (*revert_ptr)->len = rx_buf->len;
			*p_buf += rx_buf->len;

			(*revert_ptr)++;
			(*rx_buf_ptr)++;
		} else {
			/* body */
			size_t body_len = aligned_end - aligned_start;
			(*revert_ptr)->len = 0;
			(*rx_buf_ptr)->buf = rx_buf->buf;
			(*rx_buf_ptr)->len = body_len;

			(*rx_buf_ptr)++;
			(*revert_ptr)++;

			/* tail */
			if (aligned_end != buf_end) {
				size_t tail_len = buf_end - aligned_end;
				(*revert_ptr)->dst_buf = (void *)aligned_end;
				(*revert_ptr)->src_buf = (*rx_buf_ptr)->buf = *p_buf;
				(*rx_buf_ptr)->len = (*revert_ptr)->len = tail_len;
				*p_buf += (*rx_buf_ptr)->len;

				(*rx_buf_ptr)++;
				(*revert_ptr)++;
			}
			return;
		}
	} else {
		(*revert_ptr)->len = 0;
		**rx_buf_ptr = *rx_buf;

		(*rx_buf_ptr)++;
		(*revert_ptr)++;
	}
}

int transceive_with_extend_buffer(const struct device *dev, const struct spi_config *config,
				  const struct spi_buf_set *tx_bufs,
				  const struct spi_buf_set *rx_bufs, int extend_count,
				  int frag_size)
{
	int ret = 0;
	struct spi_et171_data *const data = dev->data;
	int cache_line_size = sys_cache_data_line_size_get();
	int new_count = rx_bufs->count + extend_count;
	int align_buf_size = ROUND_UP(frag_size, cache_line_size);

	ret = allocate_dma_buffers(&data->dma_buf_ctx, new_count, align_buf_size, cache_line_size);

	if (ret != 0) {
		free_dma_buffers(&data->dma_buf_ctx);
		data->aligned_rx_bufs.buffers = NULL;
		data->aligned_rx_bufs.count = 0;
		goto out;
	}

	data->dma_buf_ctx.count = new_count;

	struct spi_buf *rx_ptr = data->dma_buf_ctx.rx_bufs;
	struct revert_info *rev_ptr = data->dma_buf_ctx.revert_infos;
	uint8_t *p_buf = (uint8_t *)data->dma_buf_ctx.align_buffer;

	for (int i = 0; i < rx_bufs->count; i++) {
		const struct spi_buf *const rx_buf = &rx_bufs->buffers[i];

		process_rx_buf(rx_buf, &p_buf, &rx_ptr, &rev_ptr, cache_line_size);
	}

	data->aligned_rx_bufs.buffers = data->dma_buf_ctx.rx_bufs;
	data->aligned_rx_bufs.count = data->dma_buf_ctx.count;
	ret = transceive(dev, config, tx_bufs, &data->aligned_rx_bufs);

out:
	return ret;
}

#endif

int spi_et171_transceive(const struct device *dev, const struct spi_config *config,
			 const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs)
{
	int ret = 0;
	struct spi_et171_data *const data = dev->data;
	struct spi_context *const ctx = &data->ctx;

	if (!spi_transfer_needed(tx_bufs, rx_bufs)) {
		return 0;
	}

	spi_context_lock(ctx, false, NULL, NULL, config);
#if !defined(CONFIG_DCACHE) || !defined(CONFIG_EGIS_SPI_DMA_MODE)
	ret = transceive(dev, config, tx_bufs, rx_bufs);
#else
	int extend_count = 0, frag_size = 0;
	int need_align = is_need_alignment(tx_bufs, rx_bufs, &extend_count, &frag_size);

	if (need_align == false) {
		ret = transceive(dev, config, tx_bufs, rx_bufs);
	} else {
		ret = transceive_with_extend_buffer(dev, config, tx_bufs, rx_bufs, extend_count,
						    frag_size);
	}
#endif
	spi_context_release(ctx, ret);
	return ret;
}

#ifdef CONFIG_SPI_ASYNC
int spi_et171_transceive_async(const struct device *dev,
				const struct spi_config *config,
				const struct spi_buf_set *tx_bufs,
				const struct spi_buf_set *rx_bufs,
				spi_callback_t cb,
				void *userdata)
{
	int ret = 0;
	struct spi_et171_data *const data = dev->data;
	struct spi_context *const ctx = &data->ctx;

	if (!spi_transfer_needed(tx_bufs, rx_bufs)) {
		return 0;
	}

	spi_context_lock(ctx, true, cb, userdata, config);
#if !defined(CONFIG_DCACHE) || !defined(CONFIG_EGIS_SPI_DMA_MODE)
	ret = transceive(dev, config, tx_bufs, rx_bufs);
#else
	int extend_count = 0, frag_size = 0;
	int need_align = is_need_alignment(tx_bufs, rx_bufs, &extend_count, &frag_size);

	if (need_align == false) {
		ret = transceive(dev, config, tx_bufs, rx_bufs);
	} else {
		ret = transceive_with_extend_buffer(dev, config, tx_bufs, rx_bufs, extend_count,
						    frag_size);
	}
#endif
	spi_context_release(ctx, ret);
	return ret;
}
#endif

int spi_et171_release(const struct device *dev,
			  const struct spi_config *config)
{

	struct spi_et171_data * const data = dev->data;

	if (!spi_context_configured(&data->ctx, config)) {
		return -EINVAL;
	}

	if (data->busy) {
		return -EBUSY;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

int spi_et171_init(const struct device *dev)
{
	const struct spi_et171_cfg * const cfg = dev->config;
	struct spi_et171_data * const data = dev->data;
	int err = 0;

	/* we should not configure the device we are running on */
	if (cfg->xip) {
		return -EINVAL;
	}

	spi_context_unlock_unconditionally(&data->ctx);

#ifdef CONFIG_EGIS_SPI_DMA_MODE
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

static DEVICE_API(spi, spi_et171_api) = {.transceive = spi_et171_transceive,
#ifdef CONFIG_SPI_ASYNC
					 .transceive_async = spi_et171_transceive_async,
#endif
#ifdef CONFIG_SPI_RTIO
					 .iodev_submit = spi_rtio_iodev_default_submit,
#endif
					 .release = spi_et171_release};

static int spi_et171_prepare_fifo_tx_data(struct spi_context *ctx, uint32_t *tx_data, int dfs)
{
	if (spi_context_tx_buf_on(ctx)) {
		if (dfs == 1) {
			*tx_data = UNALIGNED_GET((const uint8_t *)ctx->tx_buf);
		} else {
			*tx_data = UNALIGNED_GET((const uint16_t *)ctx->tx_buf);
		}
	} else if (spi_context_tx_on(ctx)) {
		*tx_data = 0;
	} else {
		return false;
	}
	return true;
}

static int spi_et171_handle_fifo_rx_data(struct spi_context *ctx, uint32_t *rx_data, int dfs)
{
	if (spi_context_rx_buf_on(ctx)) {
		if (dfs == 1) {
			UNALIGNED_PUT(*rx_data, (uint8_t *)ctx->rx_buf);
		} else {
			UNALIGNED_PUT(*rx_data, (uint16_t *)ctx->rx_buf);
		}
		return true;
	} else if (!spi_context_rx_on(ctx)) {
		return false;
	} else {
		return true;
	}
}

#ifdef CONFIG_EGIS_SPI_DMA_MODE
static void spi_et171_dma_finalize(const struct device *dev)
{
	struct spi_et171_data *const data = dev->data;

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

#ifdef CONFIG_DCACHE
	if (data->dma_buf_ctx.count) {
		revert_dma_buffers(data->dma_buf_ctx.revert_infos, data->dma_buf_ctx.count);
		free_dma_buffers(&data->dma_buf_ctx);
		data->aligned_rx_bufs.buffers = NULL;
		data->aligned_rx_bufs.count = 0;
	}
#endif
}
#endif /* CONFIG_EGIS_SPI_DMA_MODE */

static void spi_et171_irq_handler(void *arg)
{
	const struct device * const dev = (const struct device *) arg;
	const struct spi_et171_cfg *const cfg = dev->config;
	struct spi_et171_data * const data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t rx_data, cur_tx_fifo_num, cur_rx_fifo_num;
	uint32_t i, intr_status;
	uint32_t tx_num = 0, tx_data = 0;
	int error = 0;
	int dfs = SPI_WORD_SIZE_GET(ctx->config->operation) >> 3;

	intr_status = sys_read32(SPI_INTST(cfg->base));

	if ((intr_status & INTST_TX_FIFO_INT_MSK) &&
	    !(intr_status & INTST_END_INT_MSK)) {

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

			if (spi_et171_prepare_fifo_tx_data(ctx, &tx_data, dfs) == true) {
				sys_write32(tx_data, SPI_DATA(cfg->base));
			} else {
				sys_clear_bits(SPI_INTEN(cfg->base), IEN_TX_FIFO_MSK);
				break;
			}

			spi_context_update_tx(ctx, dfs, 1);

			data->tx_cnt++;
		}
		sys_write32(INTST_TX_FIFO_INT_MSK, SPI_INTST(cfg->base));
	}

	if (intr_status & INTST_RX_FIFO_INT_MSK) {
		cur_rx_fifo_num = GET_RX_NUM(cfg->base);

		for (i = cur_rx_fifo_num; i > 0; i--) {

			rx_data = sys_read32(SPI_DATA(cfg->base));

			if (spi_et171_handle_fifo_rx_data(ctx, &rx_data, dfs) == false) {
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

#ifdef CONFIG_EGIS_SPI_DMA_MODE
		spi_et171_dma_finalize(dev);
#endif /* CONFIG_EGIS_SPI_DMA_MODE */

		data->busy = false;

		spi_context_complete(ctx, dev, error);
	}
}

#if CONFIG_EGIS_SPI_DMA_MODE

#define EGIS_DMA_CONFIG_DIRECTION(config)            (FIELD_GET(GENMASK(1, 0), config))
#define EGIS_DMA_CONFIG_PERIPHERAL_ADDR_INC(config)  (FIELD_GET(BIT(2), config))
#define EGIS_DMA_CONFIG_MEMORY_ADDR_INC(config)      (FIELD_GET(BIT(3), config))
#define EGIS_DMA_CONFIG_PERIPHERAL_DATA_SIZE(config) (1 << (FIELD_GET(GENMASK(6, 4), config)))
#define EGIS_DMA_CONFIG_MEMORY_DATA_SIZE(config)     (1 << (FIELD_GET(GENMASK(9, 7), config)))
#define EGIS_DMA_CONFIG_PRIORITY(config)             (FIELD_GET(BIT(10), config))

#define DMA_CHANNEL_CONFIG(id, dir)						\
		DT_INST_DMAS_CELL_BY_NAME(id, dir, channel_config)

#define SPI_DMA_CHANNEL_INIT(index, dir, dir_cap, src_dev, dest_dev)                               \
	.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(index, dir)),                           \
	.channel = DT_INST_DMAS_CELL_BY_NAME(index, dir, channel),                                 \
	.dma_cfg =                                                                                 \
		{                                                                                  \
			.dma_slot = DT_INST_DMAS_CELL_BY_NAME(index, dir, slot),                   \
			.channel_direction =                                                       \
				EGIS_DMA_CONFIG_DIRECTION(DMA_CHANNEL_CONFIG(index, dir)),        \
			.complete_callback_en = 0,                                                 \
			.error_callback_dis = 0,                                                   \
			.source_data_size = EGIS_DMA_CONFIG_##src_dev##_DATA_SIZE(                \
				DMA_CHANNEL_CONFIG(index, dir)),                                   \
			.dest_data_size = EGIS_DMA_CONFIG_##dest_dev##_DATA_SIZE(                 \
				DMA_CHANNEL_CONFIG(index, dir)),                                   \
			.source_burst_length = 1, /* SINGLE transfer */                            \
			.dest_burst_length = 1,   /* SINGLE transfer */                            \
			.channel_priority =                                                        \
				EGIS_DMA_CONFIG_PRIORITY(DMA_CHANNEL_CONFIG(index, dir)),         \
			.source_chaining_en =                                                      \
				DT_PROP(DT_INST_DMAS_CTLR_BY_NAME(index, dir), chain_transfer),    \
			.dest_chaining_en =                                                        \
				DT_PROP(DT_INST_DMAS_CTLR_BY_NAME(index, dir), chain_transfer),    \
	},                                                                                         \
	.src_addr_increment =                                                                      \
		EGIS_DMA_CONFIG_##src_dev##_ADDR_INC(DMA_CHANNEL_CONFIG(index, dir)),             \
	.dst_addr_increment =                                                                      \
		EGIS_DMA_CONFIG_##dest_dev##_ADDR_INC(DMA_CHANNEL_CONFIG(index, dir))

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

#define SPI_INIT(n)                                                                                \
	static struct spi_et171_data spi_et171_dev_data_##n = {                                    \
		SPI_CONTEXT_INIT_LOCK(spi_et171_dev_data_##n, ctx),                                \
		SPI_CONTEXT_INIT_SYNC(spi_et171_dev_data_##n, ctx),                                \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)                               \
			SPI_BUSY_INIT SPI_DMA_CHANNEL(n, rx, RX, PERIPHERAL, MEMORY)               \
				SPI_DMA_CHANNEL(n, tx, TX, MEMORY, PERIPHERAL)};                   \
	static void spi_et171_cfg_##n(void);                                                       \
	static struct spi_et171_cfg spi_et171_dev_cfg_##n = {                                      \
		.cfg_func = spi_et171_cfg_##n,                                                     \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.irq_num = DT_INST_IRQN(n),                                                        \
		.f_sys = DT_INST_PROP(n, clock_frequency),                                         \
		.xip = SPI_ROM_CFG_XIP(DT_DRV_INST(n)),                                            \
	};                                                                                         \
                                                                                                   \
	SPI_DEVICE_DT_INST_DEFINE(n, spi_et171_init, NULL, &spi_et171_dev_data_##n,                \
				  &spi_et171_dev_cfg_##n, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,   \
				  &spi_et171_api);                                                 \
                                                                                                   \
	static void spi_et171_cfg_##n(void)                                                        \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), spi_et171_irq_handler,      \
			    DEVICE_DT_INST_GET(n), 0);                                             \
	};

DT_INST_FOREACH_STATUS_OKAY(SPI_INIT)
