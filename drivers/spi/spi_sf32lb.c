/*
 * Copyright (c) 2025, Qingsong Gou <gouqs@hotmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb_spi

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/clock_control/sf32lb.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/dma/sf32lb.h>
#include <zephyr/logging/log.h>

#include <register.h>

LOG_MODULE_REGISTER(spi_sf32lb, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"

#define SPI_TOP_CTRL     offsetof(SPI_TypeDef, TOP_CTRL)
#define SPI_INTE         offsetof(SPI_TypeDef, INTE)
#define SPI_DATA         offsetof(SPI_TypeDef, DATA)
#define SPI_STATUS       offsetof(SPI_TypeDef, STATUS)
#define SPI_CLK_CTRL     offsetof(SPI_TypeDef, CLK_CTRL)
#define SPI_TRIWIRE_CTRL offsetof(SPI_TypeDef, TRIWIRE_CTRL)
#define SPI_FIFO_CTRL    offsetof(SPI_TypeDef, FIFO_CTRL)

#define SPI_FLAG_FRLVL  (SPI_STATUS_RFL_Msk | SPI_STATUS_RNE_Msk)
#define SPI_FRLVL_EMPTY (SPI_STATUS_RFL_Msk)

#define SPI_MAX_BUSY_WAIT_US 1000U

/* DMA status flags */
#define SPI_SF32LB_DMA_TX_DONE_FLAG (BIT(0))
#define SPI_SF32LB_DMA_RX_DONE_FLAG (BIT(1))
#define SPI_SF32LB_DMA_ERROR_FLAG   (BIT(2))

struct dma_stream {
	struct dma_config dma_cfg;
	struct dma_block_config dma_blk_cfg;
};

struct spi_sf32lb_config {
	uintptr_t base;
	struct sf32lb_clock_dt_spec clock;
	const struct pinctrl_dev_config *pcfg;
#ifdef CONFIG_SPI_ASYNC
	void (*irq_config_func)(void);
#endif
	bool dma_used;
	struct sf32lb_dma_dt_spec tx_dma;
	struct sf32lb_dma_dt_spec rx_dma;
};

struct spi_sf32lb_data {
	struct spi_context ctx;
	struct dma_stream dma_rx;
	struct dma_stream dma_tx;
	struct k_sem status_sem;
	uint32_t dma_status_flags;
};

static bool spi_sf32lb_transfer_ongoing(struct spi_sf32lb_data *data)
{
	return spi_context_tx_on(&data->ctx) || spi_context_rx_on(&data->ctx);
}

#ifdef CONFIG_SPI_ASYNC
void spi_sf32lb_complete(const struct device *dev, int status)
{
	const struct spi_sf32lb_config *cfg = dev->config;
	struct spi_sf32lb_data *data = dev->data;

	sys_set_bits(cfg->base + SPI_STATUS, SPI_STATUS_ROR | SPI_STATUS_TUR);

	sys_clear_bits(cfg->base + SPI_INTE, SPI_INTE_RIE | SPI_INTE_TIE);

	spi_context_complete(&data->ctx, dev, status);
}

static void spi_sf32lb_isr(const struct device *dev)
{
	const struct spi_sf32lb_config *cfg = dev->config;
	struct spi_sf32lb_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t status = sys_read32(cfg->base + SPI_STATUS);
	uint16_t tx_frame, rx_frame;
	uint8_t word_size = SPI_WORD_SIZE_GET(ctx->config->operation);

	if (status & (SPI_STATUS_ROR | SPI_STATUS_TUR)) {
		spi_sf32lb_complete(dev, -EIO);
		return;
	}

	if (IS_BIT_SET(status, SPI_STATUS_RFS_Pos) && spi_context_rx_buf_on(ctx)) {
		if (word_size == 8) {
			rx_frame = sys_read8(cfg->base + SPI_DATA);
			UNALIGNED_PUT(rx_frame, (uint8_t *)data->ctx.rx_buf);
			spi_context_update_rx(ctx, 1, 1);
		} else {
			rx_frame = sys_read32(cfg->base + SPI_DATA);
			UNALIGNED_PUT(rx_frame, (uint16_t *)data->ctx.rx_buf);
			spi_context_update_rx(ctx, 2, 1);
		}

		if (!spi_context_rx_buf_on(ctx)) {
			sys_clear_bit(cfg->base + SPI_INTE, SPI_INTE_RIE_Pos);
		}
	}

	if (IS_BIT_SET(status, SPI_STATUS_TNF_Pos) && spi_context_tx_buf_on(ctx)) {
		if (word_size == 8) {
			tx_frame = UNALIGNED_GET((uint8_t *)(data->ctx.tx_buf));
			sys_write8(tx_frame, cfg->base + SPI_DATA);
			spi_context_update_tx(ctx, 1, 1);
		} else {
			tx_frame = UNALIGNED_GET((uint16_t *)(data->ctx.tx_buf));
			sys_write32(tx_frame, cfg->base + SPI_DATA);
			spi_context_update_tx(ctx, 2, 1);
		}

		if (!spi_context_tx_buf_on(ctx)) {
			sys_clear_bit(cfg->base + SPI_INTE, SPI_INTE_TIE_Pos);
		}
	}

	if (!spi_sf32lb_transfer_ongoing(data)) {
		spi_sf32lb_complete(dev, 0);
	}
}
#endif

static int spi_sf32lb_configure(const struct device *dev, const struct spi_config *config)
{
	const struct spi_sf32lb_config *cfg = dev->config;
	struct spi_sf32lb_data *data = dev->data;
	int ret;
	uint32_t top_ctrl = 0;
	uint32_t clk_ctrl;
	uint32_t triwire_ctrl = 0;
	uint32_t clk_div;
	uint32_t clk_freq;
	uint8_t word_size;

	if (spi_context_configured(&data->ctx, config)) {
		return 0;
	}

	ret = sf32lb_clock_control_get_rate_dt(&cfg->clock, &clk_freq);
	if (ret < 0) {
		return ret;
	}

	if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_SLAVE) {
		top_ctrl |= SPI_TOP_CTRL_SFRMDIR | SPI_TOP_CTRL_SCLKDIR;
	}
	top_ctrl |= (config->operation & SPI_MODE_CPOL) ? SPI_TOP_CTRL_SPO : 0U;
	top_ctrl |= (config->operation & SPI_MODE_CPHA) ? SPI_TOP_CTRL_SPH : 0U;

	word_size = SPI_WORD_SIZE_GET(config->operation);
	if (word_size == 8U) {
		top_ctrl |= FIELD_PREP(SPI_TOP_CTRL_DSS_Msk, 8U - 1U);
	} else if (word_size == 16U) {
		top_ctrl |= FIELD_PREP(SPI_TOP_CTRL_DSS_Msk, 16U - 1U);
	} else {
		LOG_ERR("Unsupported word size: %u", word_size);
		return -ENOTSUP;
	}

	if (SPI_FRAME_FORMAT_TI == (config->operation & SPI_FRAME_FORMAT_TI)) {
		top_ctrl |= FIELD_PREP(SPI_TOP_CTRL_FRF_Msk, 0x00000001U);
	} else {
		top_ctrl |= FIELD_PREP(SPI_TOP_CTRL_FRF_Msk, 0x00000000U);
	}

	if (SPI_HALF_DUPLEX == (config->operation & SPI_HALF_DUPLEX)) {
		triwire_ctrl |= SPI_TRIWIRE_CTRL_SPI_TRI_WIRE_EN;
		top_ctrl |= SPI_TOP_CTRL_TTE;
	} else {
		triwire_ctrl &= ~SPI_TRIWIRE_CTRL_SPI_TRI_WIRE_EN;
		top_ctrl &= ~SPI_TOP_CTRL_TTE;
	}

	if (config->operation & SPI_HOLD_ON_CS) {
		return -ENOTSUP;
	}

	if (config->operation & SPI_LOCK_ON) {
		return -ENOTSUP;
	}

	sys_clear_bit(cfg->base + SPI_TOP_CTRL, SPI_TOP_CTRL_SSE_Pos);

	sys_write32(top_ctrl, cfg->base + SPI_TOP_CTRL);
	sys_write32(triwire_ctrl, cfg->base + SPI_TRIWIRE_CTRL);

	sys_set_bit(cfg->base + SPI_CLK_CTRL, SPI_CLK_CTRL_CLK_SSP_EN_Pos);
	clk_div = DIV_ROUND_UP(clk_freq,
			       config->frequency); /* see Manual 7.2.6.2.4 clock freq settings */
	clk_ctrl = sys_read32(cfg->base + SPI_CLK_CTRL);
	clk_ctrl &= ~SPI_CLK_CTRL_CLK_DIV_Msk;
	clk_ctrl |= FIELD_PREP(SPI_CLK_CTRL_CLK_DIV_Msk, clk_div);
	sys_write32(clk_ctrl, cfg->base + SPI_CLK_CTRL);

	/* Issue 1401: Make SPO setting is valid before start transfer data*/
	sys_set_bit(cfg->base + SPI_TOP_CTRL, SPI_TOP_CTRL_SSE_Pos);
	sys_clear_bit(cfg->base + SPI_TOP_CTRL, SPI_TOP_CTRL_SSE_Pos);

	data->ctx.config = config;

	return ret;
}

static void spi_sf32lb_dma_done(const struct device *dev, void *arg, uint32_t channel, int status)
{
	const struct device *spi_dev = arg;
	const struct spi_sf32lb_config *cfg = spi_dev->config;
	struct spi_sf32lb_data *data = spi_dev->data;

	if (status < 0) {
		LOG_ERR("DMA callback error with channel %d, status %d", channel, status);
		data->dma_status_flags |= SPI_SF32LB_DMA_ERROR_FLAG;
	} else {
		if (channel == cfg->tx_dma.channel) {
			data->dma_status_flags |= SPI_SF32LB_DMA_TX_DONE_FLAG;
			sf32lb_dma_stop_dt(&cfg->tx_dma);
		} else if (channel == cfg->rx_dma.channel) {
			data->dma_status_flags |= SPI_SF32LB_DMA_RX_DONE_FLAG;
			sf32lb_dma_stop_dt(&cfg->rx_dma);
		} else {
			LOG_ERR("Unknown DMA channel %d", channel);
			return;
		}
	}

	/* Check if all DMA transfers are completed */
	if ((data->dma_status_flags &
	     (SPI_SF32LB_DMA_TX_DONE_FLAG | SPI_SF32LB_DMA_RX_DONE_FLAG)) ==
	    (SPI_SF32LB_DMA_TX_DONE_FLAG | SPI_SF32LB_DMA_RX_DONE_FLAG)) {
		k_sem_give(&data->status_sem);
	}
}

static int wait_dma_rx_tx_done(const struct device *dev)
{
	struct spi_sf32lb_data *data = dev->data;
	int ret;

	/* Wait for DMA transfer completion with timeout */
	ret = k_sem_take(&data->status_sem, K_MSEC(SPI_MAX_BUSY_WAIT_US));
	if (ret < 0) {
		LOG_ERR("DMA transfer timed out");
		return -ETIMEDOUT;
	}

	/* Check DMA transfer status */
	if (data->dma_status_flags & SPI_SF32LB_DMA_ERROR_FLAG) {
		LOG_ERR("DMA transfer error");
		ret = -EIO;
	}

	/* Reset DMA status flags for next transfer */
	data->dma_status_flags = 0;

	return ret;
}

static int spi_sf32lb_dma_tx_load(const struct device *dev, const uint8_t *tx_buf, size_t len)
{
	const struct spi_sf32lb_config *cfg = dev->config;
	struct spi_sf32lb_data *data = dev->data;
	struct dma_config *tx_dma_cfg = &data->dma_tx.dma_cfg;
	struct dma_block_config *tx_dma_blk = &data->dma_tx.dma_blk_cfg;
	int ret;

	sf32lb_dma_config_init_dt(&cfg->tx_dma, tx_dma_cfg);

	tx_dma_cfg->channel_direction = MEMORY_TO_PERIPHERAL;
	tx_dma_cfg->block_count = 1U;
	tx_dma_cfg->complete_callback_en = true;
	tx_dma_cfg->dma_callback = spi_sf32lb_dma_done;
	tx_dma_cfg->user_data = (void *)dev;

	tx_dma_cfg->head_block = tx_dma_blk;

	tx_dma_blk->source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	tx_dma_blk->dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	tx_dma_blk->block_size = len;
	tx_dma_blk->source_address = (uint32_t)tx_buf;
	tx_dma_blk->dest_address = (uint32_t)(cfg->base + SPI_DATA);

	ret = sf32lb_dma_config_dt(&cfg->tx_dma, tx_dma_cfg);
	if (ret < 0) {
		LOG_ERR("Error configuring TX DMA (%d)", ret);
		return ret;
	}

	ret = sf32lb_dma_start_dt(&cfg->tx_dma);
	if (ret < 0) {
		LOG_ERR("Error starting TX DMA (%d)", ret);
		return ret;
	}

	return ret;
}

static int spi_sf32lb_dma_rx_load(const struct device *dev, uint8_t *rx_buf, size_t len)
{
	const struct spi_sf32lb_config *cfg = dev->config;
	struct spi_sf32lb_data *data = dev->data;
	struct dma_config *rx_dma_cfg = &data->dma_rx.dma_cfg;
	struct dma_block_config *rx_dma_blk = &data->dma_rx.dma_blk_cfg;
	int ret;

	sf32lb_dma_config_init_dt(&cfg->rx_dma, rx_dma_cfg);

	rx_dma_cfg->channel_direction = PERIPHERAL_TO_MEMORY;
	rx_dma_cfg->block_count = 1U;
	rx_dma_cfg->complete_callback_en = true;
	rx_dma_cfg->dma_callback = spi_sf32lb_dma_done;
	rx_dma_cfg->user_data = (void *)dev;

	rx_dma_cfg->head_block = rx_dma_blk;

	rx_dma_blk->source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	rx_dma_blk->dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	rx_dma_blk->block_size = len;
	rx_dma_blk->source_address = (uint32_t)(cfg->base + SPI_DATA);
	rx_dma_blk->dest_address = (uint32_t)rx_buf;

	ret = sf32lb_dma_config_dt(&cfg->rx_dma, rx_dma_cfg);
	if (ret < 0) {
		LOG_ERR("Error configuring RX DMA (%d)", ret);
		return ret;
	}

	ret = sf32lb_dma_start_dt(&cfg->rx_dma);
	if (ret < 0) {
		LOG_ERR("Error starting RX DMA (%d)", ret);
		return ret;
	}

	return ret;
}

static int spi_sf32lb_transceive_dma_chunk(const struct device *dev, size_t len)
{
	struct spi_sf32lb_data *data = dev->data;
	int ret;

	ret = spi_sf32lb_dma_tx_load(dev, data->ctx.tx_buf, len);
	if (ret < 0) {
		LOG_ERR("Error loading TX DMA (%d)", ret);
		return ret;
	}

	ret = spi_sf32lb_dma_rx_load(dev, data->ctx.rx_buf, len);
	if (ret < 0) {
		LOG_ERR("Error loading RX DMA (%d)", ret);
		return ret;
	}

	return ret;
}

static int spi_sf32lb_transceive_dma(const struct device *dev, const struct spi_config *config,
				     const struct spi_buf_set *tx_bufs,
				     const struct spi_buf_set *rx_bufs)
{
	const struct spi_sf32lb_config *cfg = dev->config;
	struct spi_sf32lb_data *data = dev->data;
	struct dma_config *rx_dma_cfg = &data->dma_rx.dma_cfg;
	struct dma_config *tx_dma_cfg = &data->dma_tx.dma_cfg;
	uint8_t dfs;
	uint32_t fifo_ctrl;
	size_t chunk_len;
	int ret;

	dfs = SPI_WORD_SIZE_GET(config->operation) >> 3;

	rx_dma_cfg->source_data_size = dfs;
	rx_dma_cfg->dest_data_size = dfs;
	tx_dma_cfg->source_data_size = dfs;
	tx_dma_cfg->dest_data_size = dfs;

	spi_context_lock(&data->ctx, false, NULL, NULL, config);

	spi_sf32lb_configure(dev, config);

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, dfs);

	sys_clear_bits(cfg->base + SPI_INTE, SPI_INTE_TIE | SPI_INTE_RIE | SPI_INTE_EBCEI |
						     SPI_INTE_TINTE | SPI_INTE_PINTE);

	if (sys_test_bit(cfg->base + SPI_TOP_CTRL, SPI_TOP_CTRL_SSE_Pos)) {
		sys_clear_bit(cfg->base + SPI_TOP_CTRL, SPI_TOP_CTRL_SSE_Pos);
	}

	fifo_ctrl = sys_read32(cfg->base + SPI_FIFO_CTRL);
	fifo_ctrl |= (SPI_FIFO_CTRL_RSRE | SPI_FIFO_CTRL_TSRE);
	sys_write32(fifo_ctrl, cfg->base + SPI_FIFO_CTRL);

	if (!sys_test_bit(cfg->base + SPI_TOP_CTRL, SPI_TOP_CTRL_SSE_Pos)) {
		sys_set_bit(cfg->base + SPI_TOP_CTRL, SPI_TOP_CTRL_SSE_Pos);
	}

	spi_context_cs_control(&data->ctx, true);

	while (data->ctx.rx_len > 0 || data->ctx.tx_len > 0) {
		chunk_len = spi_context_max_continuous_chunk(&data->ctx);

		/* Reset DMA status flags for new transfer */
		data->dma_status_flags = 0;

		ret = spi_sf32lb_transceive_dma_chunk(dev, chunk_len);
		if (ret < 0) {
			break;
		}

		ret = wait_dma_rx_tx_done(dev);
		if (ret != 0) {
			break;
		}

		spi_context_update_tx(&data->ctx, dfs, chunk_len);
		spi_context_update_rx(&data->ctx, dfs, chunk_len);
	}

	spi_context_cs_control(&data->ctx, false);

	spi_context_release(&data->ctx, ret);

	return ret;
}

static void spi_sf32lb_flush_rx_fifo(const struct device *dev)
{
	const struct spi_sf32lb_config *cfg = dev->config;
	uint32_t spi_status = sys_read32(cfg->base + SPI_STATUS);

	while ((spi_status & SPI_FLAG_FRLVL) != SPI_FRLVL_EMPTY) {
		(void)sys_read32(cfg->base + SPI_DATA);
		spi_status = sys_read32(cfg->base + SPI_STATUS);
	}
}

static void spi_sf32lb_reset_fifos(const struct device *dev)
{
	const struct spi_sf32lb_config *cfg = dev->config;

	/* Pulse both TX and RX FIFO reset bits to clear residual data */
	sys_set_bits(cfg->base + SPI_FIFO_CTRL, SPI_FIFO_CTRL_TSRE | SPI_FIFO_CTRL_RSRE);
	sys_clear_bits(cfg->base + SPI_FIFO_CTRL, SPI_FIFO_CTRL_TSRE | SPI_FIFO_CTRL_RSRE);
}

static int spi_sf32lb_wait_not_busy(const struct device *dev)
{
	const struct spi_sf32lb_config *cfg = dev->config;

	if (!WAIT_FOR(!sys_test_bit(cfg->base + SPI_STATUS, SPI_STATUS_BSY_Pos),
		SPI_MAX_BUSY_WAIT_US, NULL)) {
		return -ETIMEDOUT;
	}

	return 0;
}

static int spi_sf32lb_shift_tx(const struct device *dev)
{
	struct spi_sf32lb_data *data = dev->data;
	const struct spi_sf32lb_config *cfg = dev->config;
	struct spi_context *ctx = &data->ctx;
	uint16_t tx_frame = 0;

	if (spi_context_tx_buf_on(ctx)) {
		if (sys_test_bit(cfg->base + SPI_STATUS, SPI_STATUS_TNF_Pos)) {
			if (SPI_WORD_SIZE_GET(ctx->config->operation) == 8) {
				tx_frame = UNALIGNED_GET((uint8_t *)(data->ctx.tx_buf));
				sys_write8(tx_frame, cfg->base + SPI_DATA);
				spi_context_update_tx(ctx, 1, 1);
			} else if (SPI_WORD_SIZE_GET(ctx->config->operation) == 16) {
				tx_frame = UNALIGNED_GET((uint16_t *)(data->ctx.tx_buf));
				sys_write32(tx_frame, cfg->base + SPI_DATA);
				spi_context_update_tx(ctx, 2, 1);
			} else {
				LOG_ERR("Unsupported word size: %u",
					SPI_WORD_SIZE_GET(ctx->config->operation));
				return -ENOTSUP;
			}
		}
	} else {
		if (sys_test_bit(cfg->base + SPI_STATUS, SPI_STATUS_TNF_Pos)) {
			if (SPI_WORD_SIZE_GET(ctx->config->operation) == 8) {
				sys_write8(tx_frame, cfg->base + SPI_DATA);
				spi_context_update_tx(ctx, 1, 1);
			} else if (SPI_WORD_SIZE_GET(ctx->config->operation) == 16) {
				sys_write32(tx_frame, cfg->base + SPI_DATA);
				spi_context_update_tx(ctx, 2, 1);
			} else {
				LOG_ERR("Unsupported word size: %u",
					SPI_WORD_SIZE_GET(ctx->config->operation));
				return -ENOTSUP;
			}
		}
	}

	return 0;
}

static int spi_sf32lb_shift_rx(const struct device *dev)
{
	struct spi_sf32lb_data *data = dev->data;
	const struct spi_sf32lb_config *cfg = dev->config;
	struct spi_context *ctx = &data->ctx;
	uint16_t rx_frame = 0;

	if (!spi_context_tx_buf_on(ctx) &&
		sys_test_bit(cfg->base + SPI_STATUS, SPI_STATUS_TNF_Pos)) {
		if (SPI_WORD_SIZE_GET(ctx->config->operation) == 8) {
			sys_write8(0U, cfg->base + SPI_DATA);
		} else if (SPI_WORD_SIZE_GET(ctx->config->operation) == 16) {
			sys_write32(0U, cfg->base + SPI_DATA);
		} else {
			LOG_ERR("Unsupported word size: %u",
				SPI_WORD_SIZE_GET(ctx->config->operation));
			return -ENOTSUP;
		}
	}

	if (spi_context_rx_buf_on(ctx)) {
		if (sys_test_bit(cfg->base + SPI_STATUS, SPI_STATUS_RNE_Pos)) {
			if (SPI_WORD_SIZE_GET(ctx->config->operation) == 8) {
				rx_frame = sys_read8(cfg->base + SPI_DATA);
				UNALIGNED_PUT(rx_frame, (uint8_t *)data->ctx.rx_buf);
				spi_context_update_rx(ctx, 1, 1);
			} else if (SPI_WORD_SIZE_GET(ctx->config->operation) == 16) {
				rx_frame = sys_read32(cfg->base + SPI_DATA);
				UNALIGNED_PUT(rx_frame, (uint16_t *)data->ctx.rx_buf);
				spi_context_update_rx(ctx, 2, 1);
			} else {
				LOG_ERR("Unsupported word size: %u",
					SPI_WORD_SIZE_GET(ctx->config->operation));
				return -ENOTSUP;
			}
		}
	} else {
		if (sys_test_bit(cfg->base + SPI_STATUS, SPI_STATUS_RNE_Pos)) {
			if (SPI_WORD_SIZE_GET(ctx->config->operation) == 8) {
				(void)sys_read8(cfg->base + SPI_DATA);
				spi_context_update_rx(ctx, 1, 1);
			} else if (SPI_WORD_SIZE_GET(ctx->config->operation) == 16) {
				(void)sys_read32(cfg->base + SPI_DATA);
				spi_context_update_rx(ctx, 2, 1);
			} else {
				LOG_ERR("Unsupported word size: %u",
					SPI_WORD_SIZE_GET(ctx->config->operation));
				return -ENOTSUP;
			}
		}
	}

	return 0;
}

static int spi_sf32lb_frame_exchange(const struct device *dev)
{
	struct spi_sf32lb_data *data = dev->data;
	const struct spi_sf32lb_config *cfg = dev->config;
	struct spi_context *ctx = &data->ctx;
	int ret = 0;

	/* Check if the SPI is already enabled */
	if (!sys_test_bit(cfg->base + SPI_TOP_CTRL, SPI_TOP_CTRL_SSE_Pos)) {
		/* Enable SPI peripheral */
		sys_set_bit(cfg->base + SPI_TOP_CTRL, SPI_TOP_CTRL_SSE_Pos);
	}

	if (spi_context_tx_on(ctx)) {
		ret = spi_sf32lb_shift_tx(dev);
		if (ret < 0) {
			return ret;
		}
	}
	if (spi_context_rx_on(ctx)) {
		ret = spi_sf32lb_shift_rx(dev);
		if (ret < 0) {
			return ret;
		}
	}

	return ret;
}

static int spi_sf32lb_transceive_poll(const struct device *dev, const struct spi_config *config,
				      const struct spi_buf_set *tx_bufs,
				      const struct spi_buf_set *rx_bufs)
{
	const struct spi_sf32lb_config *cfg = dev->config;
	struct spi_sf32lb_data *data = dev->data;
	uint8_t dfs;
	int ret;

	spi_context_lock(&data->ctx, false, NULL, NULL, config);

	ret = spi_sf32lb_configure(dev, config);
	if (ret < 0) {
		spi_context_release(&data->ctx, ret);
		return ret;
	}

	dfs = SPI_WORD_SIZE_GET(config->operation) >> 3;
	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, dfs);

	spi_context_cs_control(&data->ctx, true);

	/* Restart peripheral to avoid residue between back-to-back transfers
	 * when the same spi_config pointer is reused (concurrent test case).
	 */
	sys_clear_bit(cfg->base + SPI_TOP_CTRL, SPI_TOP_CTRL_SSE_Pos);
	sys_set_bit(cfg->base + SPI_TOP_CTRL, SPI_TOP_CTRL_SSE_Pos);

	spi_sf32lb_reset_fifos(dev);
	spi_sf32lb_flush_rx_fifo(dev);
	sys_set_bits(cfg->base + SPI_STATUS, SPI_STATUS_ROR | SPI_STATUS_TUR | SPI_STATUS_TINT);
	do {
		ret = spi_sf32lb_frame_exchange(dev);
		if (ret < 0) {
			break;
		}
	} while (spi_sf32lb_transfer_ongoing(data));

	if (ret == 0U) {
		ret = spi_sf32lb_wait_not_busy(dev);
	}

	spi_context_cs_control(&data->ctx, false);

	spi_context_release(&data->ctx, ret);

	return ret;
}

static int spi_sf32lb_transceive(const struct device *dev, const struct spi_config *config,
				 const struct spi_buf_set *tx_bufs,
				 const struct spi_buf_set *rx_bufs)
{
	const struct spi_sf32lb_config *cfg = dev->config;

	if (tx_bufs == NULL && rx_bufs == NULL) {
		return 0;
	}

	if (cfg->dma_used) {
		return spi_sf32lb_transceive_dma(dev, config, tx_bufs, rx_bufs);
	} else {
		return spi_sf32lb_transceive_poll(dev, config, tx_bufs, rx_bufs);
	}
}

#ifdef CONFIG_SPI_ASYNC
static int spi_sf32lb_transceive_async(const struct device *dev, const struct spi_config *config,
				       const struct spi_buf_set *tx_bufs,
				       const struct spi_buf_set *rx_bufs, spi_callback_t cb,
				       void *userdata)
{
	const struct spi_sf32lb_config *cfg = dev->config;
	struct spi_sf32lb_data *data = dev->data;
	uint8_t dfs;
	int ret;

	spi_context_lock(&data->ctx, true, cb, userdata, config);

	ret = spi_sf32lb_configure(dev, config);
	if (ret < 0) {
		spi_context_release(&data->ctx, ret);
		return ret;
	}

	dfs = SPI_WORD_SIZE_GET(config->operation) >> 3;
	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, dfs);
	spi_context_cs_control(&data->ctx, true);

	sys_set_bits(cfg->base + SPI_STATUS, SPI_STATUS_ROR | SPI_STATUS_TUR);

	sys_clear_bits(cfg->base + SPI_INTE, SPI_INTE_RIE | SPI_INTE_TIE);

	if (spi_context_tx_buf_on(&data->ctx)) {
		sys_set_bit(cfg->base + SPI_INTE, SPI_INTE_TIE_Pos);
	}
	if (spi_context_rx_buf_on(&data->ctx)) {
		sys_set_bit(cfg->base + SPI_INTE, SPI_INTE_RIE_Pos);
	}

	/* Enable error interrupt */
	sys_set_bit(cfg->base + SPI_INTE, SPI_INTE_TINTE_Pos);

	/* Enable SPI peripheral if not already enabled */
	if (!sys_test_bit(cfg->base + SPI_TOP_CTRL, SPI_TOP_CTRL_SSE_Pos)) {
		sys_set_bit(cfg->base + SPI_TOP_CTRL, SPI_TOP_CTRL_SSE_Pos);
	}

	ret = spi_context_wait_for_completion(&data->ctx);

	spi_context_cs_control(&data->ctx, false);

	spi_context_release(&data->ctx, ret);

	return ret;
}
#endif

static int spi_sf32lb_release(const struct device *dev, const struct spi_config *config)
{
	struct spi_sf32lb_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(spi, spi_sf32lb_api) = {
	.transceive = spi_sf32lb_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_sf32lb_transceive_async,
#endif
	.release = spi_sf32lb_release,
};

static int spi_sf32lb_init(const struct device *dev)
{
	const struct spi_sf32lb_config *cfg = dev->config;
	struct spi_sf32lb_data *data = dev->data;
	int err;

	if (cfg->dma_used) {
		if (!sf32lb_dma_is_ready_dt(&cfg->tx_dma)) {
			LOG_ERR("TX DMA device not ready");
			return -ENODEV;
		}

		if (!sf32lb_dma_is_ready_dt(&cfg->rx_dma)) {
			LOG_ERR("RX DMA device not ready");
			return -ENODEV;
		}

		k_sem_init(&data->status_sem, 0, 1);
	}

	if (!sf32lb_clock_is_ready_dt(&cfg->clock)) {
		LOG_ERR("Clock control device not ready");
		return -ENODEV;
	}

	err = sf32lb_clock_control_on_dt(&cfg->clock);
	if (err < 0) {
		LOG_ERR("Failed to enable clock");
		return err;
	}

	err = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		LOG_ERR("Failed to set pinctrl");
		return err;
	}

	err = spi_context_cs_configure_all(&data->ctx);
	if (err < 0) {
		return err;
	}

	spi_context_unlock_unconditionally(&data->ctx);
#ifdef CONFIG_SPI_ASYNC
	cfg->irq_config_func();
#endif

	return err;
}

#define SPI_SF32LB_DEFINE(n)                                                                       \
	IF_ENABLED(CONFIG_SPI_ASYNC,                                                               \
		(static void spi_sf32lb_irq_config_func_##n(void);))                               \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static struct spi_sf32lb_data spi_sf32lb_data_##n = {                                      \
		SPI_CONTEXT_INIT_LOCK(spi_sf32lb_data_##n, ctx),                                   \
		SPI_CONTEXT_INIT_SYNC(spi_sf32lb_data_##n, ctx),                                   \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)};                             \
                                                                                                   \
	static const struct spi_sf32lb_config spi_sf32lb_config_##n = {                            \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.clock = SF32LB_CLOCK_DT_INST_SPEC_GET(n),                                         \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		IF_ENABLED(CONFIG_SPI_ASYNC,                                                       \
			(.irq_config_func = spi_sf32lb_irq_config_func_##n,))                      \
		.dma_used = DT_INST_NODE_HAS_PROP(n, dmas),                                        \
		.tx_dma = SF32LB_DMA_DT_INST_SPEC_GET_BY_NAME_OR(n, tx, {}),                       \
		.rx_dma = SF32LB_DMA_DT_INST_SPEC_GET_BY_NAME_OR(n, rx, {}),                       \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, spi_sf32lb_init, NULL, &spi_sf32lb_data_##n,                      \
			      &spi_sf32lb_config_##n, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,       \
			      &spi_sf32lb_api);                                                    \
	IF_ENABLED(CONFIG_SPI_ASYNC,                                                               \
		(static void spi_sf32lb_irq_config_func_##n(void)                                  \
		{                                                                                  \
			IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), spi_sf32lb_isr,     \
			    DEVICE_DT_INST_GET(n), 0);                                             \
			irq_enable(DT_INST_IRQN(n));                                               \
		}))

DT_INST_FOREACH_STATUS_OKAY(SPI_SF32LB_DEFINE)
