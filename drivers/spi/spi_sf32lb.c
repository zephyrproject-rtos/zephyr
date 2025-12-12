/*
 * Copyright (c) 2025, Qingsong Gou <gouqs@hotmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb_spi

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/clock_control/sf32lb.h>
#include <zephyr/drivers/pinctrl.h>
#ifdef CONFIG_SPI_SF32LB_DMA
#include <zephyr/drivers/dma/sf32lb.h>
#endif
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

struct spi_sf32lb_config {
	uintptr_t base;
	struct sf32lb_clock_dt_spec clock;
	const struct pinctrl_dev_config *pcfg;
#ifdef CONFIG_SPI_SF32LB_INTERRUPT
	void (*irq_config_func)(void);
#endif
#ifdef CONFIG_SPI_SF32LB_DMA
	struct sf32lb_dma_dt_spec tx_dma;
	struct sf32lb_dma_dt_spec rx_dma;
#endif
};

struct spi_sf32lb_data {
	struct spi_context ctx;
};

static bool spi_sf32lb_transfer_ongoing(struct spi_sf32lb_data *data)
{
	return spi_context_tx_on(&data->ctx) || spi_context_rx_on(&data->ctx);
}

#ifdef CONFIG_SPI_SF32LB_INTERRUPT
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
#else
static int spi_sf32lb_frame_exchange(const struct device *dev)
{
	struct spi_sf32lb_data *data = dev->data;
	const struct spi_sf32lb_config *cfg = dev->config;
	struct spi_context *ctx = &data->ctx;
	uint16_t tx_frame, rx_frame;

	/* Check if the SPI is already enabled */
	if (!sys_test_bit(cfg->base + SPI_TOP_CTRL, SPI_TOP_CTRL_SSE_Pos)) {
		/* Enable SPI peripheral */
		sys_set_bit(cfg->base + SPI_TOP_CTRL, SPI_TOP_CTRL_SSE_Pos);
	}

	if (SPI_WORD_SIZE_GET(ctx->config->operation) == 8) {
		if (spi_context_tx_buf_on(ctx) &&
			sys_test_bit(cfg->base + SPI_STATUS, SPI_STATUS_TNF_Pos)) {
			tx_frame = UNALIGNED_GET((uint8_t *)(data->ctx.tx_buf));
			sys_write8(tx_frame, cfg->base + SPI_DATA);
			spi_context_update_tx(ctx, 1, 1);
		}
	} else {
		if (spi_context_tx_buf_on(ctx) &&
			sys_test_bit(cfg->base + SPI_STATUS, SPI_STATUS_TNF_Pos)) {
			tx_frame = UNALIGNED_GET((uint16_t *)(data->ctx.tx_buf));
			sys_write32(tx_frame, cfg->base + SPI_DATA);
			spi_context_update_tx(ctx, 2, 1);
		}
	}

	if (SPI_WORD_SIZE_GET(ctx->config->operation) == 8) {
		if (spi_context_rx_buf_on(ctx) &&
			sys_test_bit(cfg->base + SPI_STATUS, SPI_STATUS_RNE_Pos)) {
			rx_frame = sys_read8(cfg->base + SPI_DATA);
			UNALIGNED_PUT(rx_frame, (uint8_t *)data->ctx.rx_buf);
			spi_context_update_rx(ctx, 1, 1);
		}
	} else {
		if (spi_context_rx_buf_on(ctx) &&
			sys_test_bit(cfg->base + SPI_STATUS, SPI_STATUS_RNE_Pos)) {
			rx_frame = sys_read32(cfg->base + SPI_DATA);
			UNALIGNED_PUT(rx_frame, (uint16_t *)data->ctx.rx_buf);
			spi_context_update_rx(ctx, 2, 1);
		}
	}

	return 0;
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
	clk_div = clk_freq / config->frequency; /* see Manual 7.2.6.2.4 clock freq settings */
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

#ifdef CONFIG_SPI_SF32LB_DMA
static void spi_sf32lb_dma_done(const struct device *dev, void *arg, uint32_t channel,
			   int status)
{
	const struct device *spi_dev = arg;
	const struct spi_sf32lb_config *cfg = spi_dev->config;
	struct spi_sf32lb_data *data = spi_dev->data;
	uint8_t dfs = SPI_WORD_SIZE_GET(data->ctx.config->operation) >> 3;

	if (channel == cfg->tx_dma.channel) {
		spi_context_update_tx(&data->ctx, dfs, data->ctx.tx_len);
		sys_clear_bit(cfg->base + SPI_INTE, SPI_INTE_TIE_Pos);
		if (!spi_context_rx_buf_on(&data->ctx)) {
			spi_context_complete(&data->ctx, spi_dev, status);
		}
	}
	if (channel == cfg->rx_dma.channel) {
		spi_context_update_rx(&data->ctx, dfs, data->ctx.rx_len);
		sys_clear_bit(cfg->base + SPI_INTE, SPI_INTE_RIE_Pos);
		if (!spi_context_tx_buf_on(&data->ctx)) {
			spi_context_complete(&data->ctx, spi_dev, status);
		}
	}

	printk("spi_sf32lb_dma_done!\n");
}

static int spi_sf32lb_transceive_dma(const struct device *dev, const struct spi_config *config,
			     const struct spi_buf_set *tx_bufs,
			     const struct spi_buf_set *rx_bufs)
{
	const struct spi_sf32lb_config *cfg = dev->config;
	struct spi_sf32lb_data *data = dev->data;
	uint8_t *tx_buf = (uint8_t *)data->ctx.tx_buf;
	uint8_t *rx_buf = (uint8_t *)data->ctx.rx_buf;
	uint32_t fifo_ctrl;
	uint32_t len;
	struct dma_config tx_dma_cfg = {0};
	struct dma_config rx_dma_cfg = {0};
	struct dma_block_config tx_dma_blk = {0};
	struct dma_block_config rx_dma_blk = {0};
	uint8_t data_size = SPI_WORD_SIZE_GET(config->operation) >> 3;
	int ret;

	len = data->ctx.rx_len ? data->ctx.rx_len : data->ctx.tx_len;

	sys_clear_bits(cfg->base + SPI_INTE, SPI_INTE_TIE | SPI_INTE_RIE | SPI_INTE_EBCEI |
				     SPI_INTE_TINTE | SPI_INTE_PINTE);

	if (sys_test_bit(cfg->base + SPI_TOP_CTRL, SPI_TOP_CTRL_SSE_Pos)) {
		sys_clear_bit(cfg->base + SPI_TOP_CTRL, SPI_TOP_CTRL_SSE_Pos);
	}

	// Configure FIFO thresholds
	fifo_ctrl = sys_read32(cfg->base + SPI_FIFO_CTRL);
	fifo_ctrl &= ~(SPI_FIFO_CTRL_RSRE | SPI_FIFO_CTRL_TSRE);

	if (spi_context_rx_buf_on(&data->ctx)) {
		sf32lb_dma_config_init_dt(&cfg->rx_dma, &rx_dma_cfg);

		rx_dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
		rx_dma_cfg.source_data_size = data_size;
		rx_dma_cfg.dest_data_size = data_size;
		rx_dma_cfg.block_count = 1U;
		rx_dma_cfg.complete_callback_en = true;
		rx_dma_cfg.dma_callback = spi_sf32lb_dma_done;
		rx_dma_cfg.user_data = (void *)dev;

		rx_dma_cfg.head_block = &rx_dma_blk;

		rx_dma_blk.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		rx_dma_blk.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		rx_dma_blk.block_size = len;
		rx_dma_blk.source_address = (uint32_t)(cfg->base + SPI_DATA);
		rx_dma_blk.dest_address = (uint32_t)rx_buf;

		ret = sf32lb_dma_config_dt(&cfg->rx_dma, &rx_dma_cfg);
		if (ret < 0) {
			LOG_ERR("Error configuring RX DMA (%d)", ret);
			return ret;
		}

		ret = sf32lb_dma_start_dt(&cfg->rx_dma);
		if (ret < 0) {
			LOG_ERR("Error starting RX DMA (%d)", ret);
			return ret;
		}
		sys_set_bit(cfg->base + SPI_INTE, SPI_INTE_RIE_Pos);
		fifo_ctrl |= SPI_FIFO_CTRL_RSRE;
		sys_write32(fifo_ctrl, cfg->base + SPI_FIFO_CTRL);
	}

	if (spi_context_tx_buf_on(&data->ctx)) {
		sf32lb_dma_config_init_dt(&cfg->tx_dma, &tx_dma_cfg);

		tx_dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
		tx_dma_cfg.source_data_size = data_size;
		tx_dma_cfg.dest_data_size = data_size;
		tx_dma_cfg.block_count = 1U;
		tx_dma_cfg.complete_callback_en = true;
		tx_dma_cfg.dma_callback = spi_sf32lb_dma_done;
		tx_dma_cfg.user_data = (void *)dev;

		tx_dma_cfg.head_block = &tx_dma_blk;

		tx_dma_blk.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		tx_dma_blk.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		tx_dma_blk.block_size = len;
		tx_dma_blk.source_address = (uint32_t)tx_buf;
		tx_dma_blk.dest_address = (uint32_t)(cfg->base + SPI_DATA);

		ret = sf32lb_dma_config_dt(&cfg->tx_dma, &tx_dma_cfg);
		if (ret < 0) {
			LOG_ERR("Error configuring TX DMA (%d)", ret);
			return ret;
		}

		ret = sf32lb_dma_start_dt(&cfg->tx_dma);
		if (ret < 0) {
			LOG_ERR("Error starting TX DMA (%d)", ret);
			return ret;
		}

		sys_set_bit(cfg->base + SPI_INTE, SPI_INTE_TIE_Pos);
		fifo_ctrl |= SPI_FIFO_CTRL_TSRE;
		sys_write32(fifo_ctrl, cfg->base + SPI_FIFO_CTRL);
	}

	//sys_write32(fifo_ctrl, cfg->base + SPI_FIFO_CTRL);

	if (!sys_test_bit(cfg->base + SPI_TOP_CTRL, SPI_TOP_CTRL_SSE_Pos)) {
		sys_set_bit(cfg->base + SPI_TOP_CTRL, SPI_TOP_CTRL_SSE_Pos);
	}

	return ret;
}
#endif /* CONFIG_SPI_SF32LB_DMA */

static int spi_sf32lb_transceive(const struct device *dev, const struct spi_config *config,
				 const struct spi_buf_set *tx_bufs,
				 const struct spi_buf_set *rx_bufs)
{
	struct spi_sf32lb_data *data = dev->data;
#ifdef CONFIG_SPI_SF32LB_INTERRUPT
	const struct spi_sf32lb_config *cfg = dev->config;
#endif
	uint8_t dfs;
	int ret;

	if (tx_bufs == NULL && rx_bufs == NULL) {
		return 0;
	}

	spi_context_lock(&data->ctx, false, NULL, NULL, config);

	ret = spi_sf32lb_configure(dev, config);
	if (ret < 0) {
		spi_context_release(&data->ctx, ret);
		return ret;
	}

	dfs = SPI_WORD_SIZE_GET(config->operation) >> 3;
	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, dfs);
	spi_context_cs_control(&data->ctx, true);

#ifdef CONFIG_SPI_SF32LB_INTERRUPT
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
#else
	do {
		ret = spi_sf32lb_frame_exchange(dev);
		if (ret < 0) {
			break;
		}
	} while (spi_sf32lb_transfer_ongoing(data));
#endif
	spi_context_cs_control(&data->ctx, false);

	spi_context_release(&data->ctx, ret);

	return ret;
}

static int spi_sf32lb_release(const struct device *dev, const struct spi_config *config)
{
	struct spi_sf32lb_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#ifdef CONFIG_SPI_ASYNC
static int spi_sf32lb_transceive_async(const struct device *dev, const struct spi_config *config,
					  const struct spi_buf_set *tx_bufs,
					  const struct spi_buf_set *rx_bufs,
					 spi_callback_t cb, void *user_data)
{
	struct spi_sf32lb_data *data = dev->data;
	//struct spi_context *ctx = &data->ctx;
	uint8_t dfs;
	int ret;

	spi_context_lock(&data->ctx, true, cb, user_data, config);

	ret = spi_sf32lb_configure(dev, config);
	if (ret < 0) {
		spi_context_release(&data->ctx, ret);
		return ret;
	}

	dfs = SPI_WORD_SIZE_GET(config->operation) >> 3;
	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, dfs);
	spi_context_cs_control(&data->ctx, true);

#if defined(CONFIG_SPI_SF32LB_DMA)
	ret = spi_sf32lb_transceive_dma(dev, config, tx_bufs, rx_bufs);
	if (ret < 0) {
		spi_context_cs_control(&data->ctx, false);
		spi_context_release(&data->ctx, ret);
		return ret;
	}
#endif
	spi_context_cs_control(&data->ctx, false);
	spi_context_release(&data->ctx, ret);

	return ret;
}
#endif

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

#ifdef CONFIG_SPI_SF32LB_DMA
	if (!sf32lb_dma_is_ready_dt(&cfg->tx_dma)) {
		LOG_ERR("TX DMA device not ready");
		return -ENODEV;
	}

	if (!sf32lb_dma_is_ready_dt(&cfg->rx_dma)) {
		LOG_ERR("RX DMA device not ready");
		return -ENODEV;
	}
#endif

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
#ifdef CONFIG_SPI_SF32LB_INTERRUPT
	cfg->irq_config_func();
#endif

	return err;
}

#define SPI_SF32LB_DEFINE(n)                                                                       \
	IF_ENABLED(CONFIG_SPI_SF32LB_INTERRUPT,                                                    \
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
		IF_ENABLED(CONFIG_SPI_SF32LB_INTERRUPT,                                            \
			(.irq_config_func = spi_sf32lb_irq_config_func_##n,))                      \
		IF_ENABLED(CONFIG_SPI_SF32LB_DMA,(                                                 \
			.tx_dma = SF32LB_DMA_DT_INST_SPEC_GET(n),                                  \
			.rx_dma = SF32LB_DMA_DT_INST_SPEC_GET(n),))                                \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, spi_sf32lb_init, NULL, &spi_sf32lb_data_##n,                      \
			      &spi_sf32lb_config_##n, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,       \
			      &spi_sf32lb_api);                                                    \
	IF_ENABLED(CONFIG_SPI_SF32LB_INTERRUPT,                                                    \
		(static void spi_sf32lb_irq_config_func_##n(void)                                  \
		{                                                                                  \
			IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), spi_sf32lb_isr,     \
			    DEVICE_DT_INST_GET(n), 0);                                             \
			irq_enable(DT_INST_IRQN(n));                                               \
		}))

DT_INST_FOREACH_STATUS_OKAY(SPI_SF32LB_DEFINE)
