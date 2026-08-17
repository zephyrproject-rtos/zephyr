/*
 * Copyright (c) 2026 Realtek Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT realtek_rts5817_spi

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_rts5817);

#include <zephyr/cache.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/minmax.h>
#include <zephyr/irq.h>
#include <errno.h>
#include "spi_dw.h"
#include "spi_rts5817.h"

#ifdef CONFIG_PINCTRL
#include <zephyr/drivers/pinctrl.h>
#endif

BUILD_ASSERT(CONFIG_SPI_RTS5817_INIT_PRIORITY > CONFIG_SPI_INIT_PRIORITY,
	     "The RTS5817 SPI wrapper must be initialized after the DesignWare SPI driver");

static void ctrl_reg_write(const struct spi_rts5817_config *cfg, uint32_t offset, uint32_t value)
{
	sys_write32(value, cfg->ctrl_regs + offset);
}

static uint32_t ctrl_reg_read(const struct spi_rts5817_config *cfg, uint32_t offset)
{
	return sys_read32(cfg->ctrl_regs + offset);
}

static void ctrl_reg_set_bits(const struct spi_rts5817_config *cfg, uint32_t offset, uint32_t bits)
{
	sys_set_bits(cfg->ctrl_regs + offset, bits);
}

static void ctrl_reg_clear_bits(const struct spi_rts5817_config *cfg, uint32_t offset,
				uint32_t bits)
{
	sys_clear_bits(cfg->ctrl_regs + offset, bits);
}

static inline bool spi_rts5817_transfer_ongoing(struct spi_rts5817_data *data)
{
	return (spi_context_tx_on(&data->ctx) || spi_context_rx_on(&data->ctx));
}

static void spi_rts5817_release_buffer(struct spi_rts5817_data *data)
{
	k_free(data->aligned_buffer);
	data->aligned_buffer = NULL;
	data->aligned_buffer_size = 0;
}

static void spi_rts5817_enable_cs(const struct device *dev)
{
	const struct spi_rts5817_config *cfg = dev->config;
	struct spi_rts5817_data *data = dev->data;

	write_ser(cfg->dw_spi_dev, BIT(0));
	if (spi_cs_is_gpio(data->ctx.config)) {
		spi_context_cs_control(&data->ctx, true);
	}
}

static void spi_rts5817_disable_cs(const struct device *dev)
{
	const struct spi_rts5817_config *cfg = dev->config;
	struct spi_rts5817_data *data = dev->data;

	write_ser(cfg->dw_spi_dev, 0);
	if (spi_cs_is_gpio(data->ctx.config)) {
		spi_context_cs_control(&data->ctx, false);
	}
}

static void completed(const struct device *dev, int err)
{
	const struct spi_rts5817_config *cfg = dev->config;
	struct spi_rts5817_data *data = dev->data;

	spi_rts5817_disable_cs(dev);

	ctrl_reg_clear_bits(cfg, R_MST_SPI_SSI_IRQ_ENABLE, MST_DONE_INT_MASK);
	clear_bit_ssienr(cfg->dw_spi_dev);

	spi_rts5817_release_buffer(data);

	LOG_DBG("SPI transaction finished %s error", err ? "with" : "without");

	spi_context_complete(&data->ctx, dev, err);
}

static int spi_rts5817_configure(const struct device *dev, const struct spi_config *config)
{

	const struct spi_rts5817_config *cfg = dev->config;
	const struct spi_dw_config *dw_cfg = cfg->dw_spi_dev->config;
	struct spi_rts5817_data *data = dev->data;
	uint32_t ctrlr0 = 0U;

	LOG_DBG("%p (prev %p)", config, data->ctx.config);

	if (spi_context_configured(&data->ctx, config)) {
		/* Nothing to do */
		return 0;
	}

	if (config->operation & SPI_HALF_DUPLEX) {
		LOG_ERR("Half-duplex not supported");
		return -ENOTSUP;
	}

	if (config->operation & SPI_OP_MODE_SLAVE) {
		LOG_ERR("Slave mode not supported");
		return -ENOTSUP;
	}

	if ((config->operation & SPI_TRANSFER_LSB) ||
	    (IS_ENABLED(CONFIG_SPI_EXTENDED_MODES) &&
	     (config->operation & (SPI_LINES_DUAL | SPI_LINES_QUAD | SPI_LINES_OCTAL)))) {
		LOG_ERR("Unsupported configuration");
		return -EINVAL;
	}

	if (SPI_WORD_SIZE_GET(config->operation) != 8) {
		LOG_ERR("Word size of %u not allowed", SPI_WORD_SIZE_GET(config->operation));
		return -ENOTSUP;
	}

	/* Word size */
	ctrlr0 |= SPI_WORD_SIZE_GET(config->operation) - 1;

	/* Determine how many bytes are required per-frame */
	data->dfs = SPI_WS_TO_DFS(SPI_WORD_SIZE_GET(config->operation));

	/* SPI mode */
	if (SPI_MODE_GET(config->operation) & SPI_MODE_CPOL) {
		ctrlr0 |= DW_SPI_CTRLR0_SCPOL;
	}

	if (SPI_MODE_GET(config->operation) & SPI_MODE_CPHA) {
		ctrlr0 |= DW_SPI_CTRLR0_SCPH;
	}

	if (SPI_MODE_GET(config->operation) & SPI_MODE_LOOP) {
		ctrlr0 |= DW_SPI_CTRLR0_SRL;
	}

	/* Installing the configuration */
	write_ctrlr0(cfg->dw_spi_dev, ctrlr0);

	LOG_DBG("CTRLR0:%x", read_ctrlr0(cfg->dw_spi_dev));

	/* At this point, it's mandatory to set this on the context! */
	data->ctx.config = config;

	/* Baud rate and Slave select, for master only */
	write_baudr(cfg->dw_spi_dev, SPI_DW_CLK_DIVIDER(data->clock_frequency, config->frequency));

	/* Config txftlr & rxftlr */
	write_txftlr(cfg->dw_spi_dev, RTS5817_SPI_TXFTLR_VALUE);
	write_rxftlr(cfg->dw_spi_dev, RTS5817_SPI_RXFTLR_VALUE);

	/* Config rx sample delay */
	if ((config->frequency >= 60000000) &&
	    (dw_cfg->pcfg->states->pins->power_source == IO_POWER_1V8)) {
		write_rx_sample_dly(cfg->dw_spi_dev, 0x2);
	} else {
		write_rx_sample_dly(cfg->dw_spi_dev, 0x1);
	}

	ctrl_reg_clear_bits(cfg, R_MST_SPI_SSI_CONTROL, MST_SCK_INTERVAL_EN_MASK);

	LOG_DBG("Installed master config %p: freq %uHz (div = %u),"
		" ws/dfs %u/%u, mode %u/%u/%u, slave %u",
		config, config->frequency,
		SPI_DW_CLK_DIVIDER(data->clock_frequency, config->frequency),
		SPI_WORD_SIZE_GET(config->operation), data->dfs,
		(SPI_MODE_GET(config->operation) & SPI_MODE_CPOL) ? 1 : 0,
		(SPI_MODE_GET(config->operation) & SPI_MODE_CPHA) ? 1 : 0,
		(SPI_MODE_GET(config->operation) & SPI_MODE_LOOP) ? 1 : 0, config->slave);

	return 0;
}

static size_t spi_rts5817_next_chunk_len(struct spi_context *ctx)
{
	if (ctx->tx_len == 0) {
		return ctx->rx_len;
	}
	if (ctx->rx_len == 0) {
		return ctx->tx_len;
	}
	return min(ctx->tx_len, ctx->rx_len);
}

static size_t spi_rts5817_max_chunk_len(const struct spi_buf_set *tx_bufs,
					const struct spi_buf_set *rx_bufs)
{
	size_t max_len = 0;
	size_t i;

	if (tx_bufs) {
		for (i = 0; i < tx_bufs->count; i++) {
			max_len = max(max_len, tx_bufs->buffers[i].len);
		}
	}
	if (rx_bufs) {
		for (i = 0; i < rx_bufs->count; i++) {
			max_len = max(max_len, rx_bufs->buffers[i].len);
		}
	}

	return max_len;
}

static void spi_rts5817_transfer_chunk(const struct device *dev)
{
	const struct spi_rts5817_config *cfg = dev->config;
	struct spi_rts5817_data *data = dev->data;
	size_t dma_len = spi_rts5817_next_chunk_len(&data->ctx);
	uint8_t *tx_buf;

	__ASSERT_NO_MSG(dma_len > 0 && dma_len <= data->aligned_buffer_size);

	if (spi_context_tx_buf_on(&data->ctx)) {
		tx_buf = (uint8_t *)data->ctx.tx_buf;
		sys_cache_data_flush_range(tx_buf, dma_len);
	} else {
		memset(data->aligned_buffer, 0, dma_len);
		tx_buf = data->aligned_buffer;
	}

	/* Pre-DMA: write back any dirty lines covering the RX target so they
	 * cannot evict over DMA-written data. The matching post-DMA invalidate
	 * happens in spi_rts5817_finish_chunk() once the transfer has completed.
	 */
	sys_cache_data_flush_range(data->aligned_buffer, dma_len);

	ctrl_reg_write(cfg, R_MST_SPI_SSI_WR_ADDR, (uint32_t)data->aligned_buffer);
	ctrl_reg_write(cfg, R_MST_SPI_SSI_RD_ADDR, (uint32_t)tx_buf);
	ctrl_reg_write(cfg, R_MST_SPI_SSI_DATA_LEN, dma_len);

	data->xfer_len = dma_len;

	/* Start transfer */
	ctrl_reg_set_bits(cfg, R_MST_SPI_SSI_START_CTRL, MST_SSI_START_MASK);
}

static void spi_rts5817_finish_chunk(const struct device *dev)
{
	struct spi_rts5817_data *data = dev->data;

	sys_cache_data_invd_range(data->aligned_buffer, data->xfer_len);

	if (spi_context_rx_buf_on(&data->ctx)) {
		memcpy(data->ctx.rx_buf, data->aligned_buffer, data->xfer_len);
	}

	spi_context_update_rx(&data->ctx, data->dfs, data->xfer_len);
	spi_context_update_tx(&data->ctx, data->dfs, data->xfer_len);
}

#ifndef CONFIG_SPI_RTS5817_INTERRUPT
static bool spi_rts5817_wait_chunk_done(const struct spi_rts5817_config *cfg)
{
	if (!WAIT_FOR((ctrl_reg_read(cfg, R_MST_SPI_SSI_START_CTRL) & MST_SSI_START_MASK) == 0,
		      RTS5817_SPI_TIME_OUT_MS * 1000, k_yield())) {
		ctrl_reg_clear_bits(cfg, R_MST_SPI_SSI_START_CTRL, MST_SSI_START_MASK);
		return false;
	}
	return true;
}
#endif

static int transceive(const struct device *dev, const struct spi_config *config,
		      const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs,
		      bool asynchronous, spi_callback_t cb, void *userdata)
{
	const struct spi_rts5817_config *cfg = dev->config;
	struct spi_rts5817_data *data = dev->data;
	size_t max_chunk;
	int err;

	if (!config) {
		return -EINVAL;
	}

	if (!tx_bufs && !rx_bufs) {
		return 0;
	}

#ifndef CONFIG_SPI_RTS5817_INTERRUPT
	if (asynchronous) {
		return -ENOTSUP;
	}
#endif

	max_chunk = spi_rts5817_max_chunk_len(tx_bufs, rx_bufs);
	if (max_chunk == 0) {
		return 0;
	}

	if (max_chunk > UINT16_MAX) {
		return -EINVAL;
	}

	spi_context_lock(&data->ctx, asynchronous, cb, userdata, config);

	err = spi_rts5817_configure(dev, config);
	if (err) {
		goto out;
	}

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, data->dfs);

	data->aligned_buffer = k_aligned_alloc(RTS5817_SPI_RX_BUFF_ALIGN_SIZE, max_chunk);
	if (data->aligned_buffer == NULL) {
		err = -ENOMEM;
		goto out;
	}
	data->aligned_buffer_size = max_chunk;

	spi_rts5817_enable_cs(dev);

#ifdef CONFIG_SPI_RTS5817_INTERRUPT
	/* Clear and enable the DONE interrupt */
	ctrl_reg_set_bits(cfg, R_MST_SPI_SSI_IRQ_STATUS, MST_DONE_INT_MASK);
	ctrl_reg_set_bits(cfg, R_MST_SPI_SSI_IRQ_ENABLE, MST_DONE_INT_MASK);
	set_bit_ssienr(cfg->dw_spi_dev);

	spi_rts5817_transfer_chunk(dev);

	/* Buffer ownership now belongs to the ISR; it frees in completed(). */
	err = spi_context_wait_for_completion(&data->ctx);
#else
	set_bit_ssienr(cfg->dw_spi_dev);

	do {
		spi_rts5817_transfer_chunk(dev);
		if (!spi_rts5817_wait_chunk_done(cfg)) {
			err = -ETIMEDOUT;
			break;
		}
		spi_rts5817_finish_chunk(dev);
	} while (spi_rts5817_transfer_ongoing(data));

	clear_bit_ssienr(cfg->dw_spi_dev);
	spi_rts5817_disable_cs(dev);
	spi_rts5817_release_buffer(data);
#endif

out:
	spi_context_release(&data->ctx, err);
	return err;
}

static int spi_rts5817_transceive(const struct device *dev, const struct spi_config *config,
				  const struct spi_buf_set *tx_bufs,
				  const struct spi_buf_set *rx_bufs)
{
	LOG_DBG("%p, %p, %p", dev, tx_bufs, rx_bufs);

	return transceive(dev, config, tx_bufs, rx_bufs, false, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int spi_rts5817_transceive_async(const struct device *dev, const struct spi_config *config,
					const struct spi_buf_set *tx_bufs,
					const struct spi_buf_set *rx_bufs, spi_callback_t cb,
					void *userdata)
{
	LOG_DBG("%p, %p, %p, %p, %p", dev, tx_bufs, rx_bufs, cb, userdata);

	return transceive(dev, config, tx_bufs, rx_bufs, true, cb, userdata);
}
#endif /* CONFIG_SPI_ASYNC */

static int spi_rts5817_release(const struct device *dev, const struct spi_config *config)
{
	struct spi_rts5817_data *data = dev->data;

	if (!spi_context_configured(&data->ctx, config)) {
		return -EINVAL;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

void spi_rts5817_isr(const struct device *dev)
{
	const struct spi_rts5817_config *cfg = dev->config;
	struct spi_rts5817_data *data = dev->data;
	uint32_t int_status;

	int_status = ctrl_reg_read(cfg, R_MST_SPI_SSI_IRQ_STATUS);

	LOG_DBG("SPI %p int_status 0x%x", dev, int_status);

	if (!(int_status & MST_DONE_INT_MASK)) {
		completed(dev, -EIO);
		return;
	}

	ctrl_reg_set_bits(cfg, R_MST_SPI_SSI_IRQ_STATUS, MST_DONE_INT_MASK);

	spi_rts5817_finish_chunk(dev);

	if (!spi_rts5817_transfer_ongoing(data)) {
		completed(dev, 0);
		return;
	}

	spi_rts5817_transfer_chunk(dev);
}

static int spi_rts5817_clock_rate_get(const struct device *dev)
{
	const struct spi_rts5817_config *cfg = dev->config;
	struct spi_rts5817_data *data = dev->data;

	if (!device_is_ready(cfg->clock_dev)) {
		return -ENODEV;
	}

	return clock_control_get_rate(cfg->clock_dev, cfg->clock_subsys, &data->clock_frequency);
}

static DEVICE_API(spi, rts5817_spi_api) = {
	.transceive = spi_rts5817_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_rts5817_transceive_async,
#endif /* CONFIG_SPI_ASYNC */
	.release = spi_rts5817_release,
};

static int spi_rts5817_init(const struct device *dev)
{
	const struct spi_rts5817_config *cfg = dev->config;
	struct spi_rts5817_data *data = dev->data;
	int err;

	if (!device_is_ready(cfg->dw_spi_dev)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->dw_spi_dev);
		return -ENODEV;
	}

	err = spi_rts5817_clock_rate_get(dev);
	if (err < 0) {
		return err;
	}

	ctrl_reg_clear_bits(cfg, R_MST_SPI_SSI_IRQ_ENABLE, SSI_MST_INT_MASK);
	ctrl_reg_set_bits(cfg, R_MST_SPI_SSI_IRQ_STATUS, SSI_MST_INT_MASK);

	clear_bit_ssienr(cfg->dw_spi_dev);
	write_imr(cfg->dw_spi_dev, 0x0);

#ifdef CONFIG_SPI_RTS5817_INTERRUPT
	cfg->irq_config_func();
#endif

	LOG_DBG("RTS5817 SPI driver initialized on device: %p", dev);

	err = spi_context_cs_configure_all(&data->ctx);
	if (err < 0) {
		return err;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#ifdef CONFIG_SPI_RTS5817_INTERRUPT
#define SPI_RTS5817_IRQ_HANDLER(inst)                                                              \
	static void spi_rts5817_irq_config_##inst(void)                                            \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), spi_rts5817_isr,      \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
		irq_enable(DT_INST_IRQN(inst));                                                    \
	}

#define SPI_RTS5817_IRQ_HANDLER_FUNC(id) .irq_config_func = spi_rts5817_irq_config_##id,
#else
#define SPI_RTS5817_IRQ_HANDLER(inst)
#define SPI_RTS5817_IRQ_HANDLER_FUNC(inst)
#endif /* CONFIG_SPI_RTS5817_INTERRUPT */

#define DW_PHANDLE(inst)        DT_INST_PHANDLE(inst, dw_spi_dev)
#define DW_CLOCKS_CTLR(inst)    DT_CLOCKS_CTLR(DW_PHANDLE(inst))
#define DW_CLOCKS_CELL(inst, c) DT_PHA(DW_PHANDLE(inst), clocks, c)

#define SPI_RTS5817_INIT(inst)                                                                     \
	SPI_RTS5817_IRQ_HANDLER(inst);                                                             \
	static struct spi_rts5817_data spi_rts5817_data_##inst = {                                 \
		SPI_CONTEXT_INIT_LOCK(spi_rts5817_data_##inst, ctx),                               \
		SPI_CONTEXT_INIT_SYNC(spi_rts5817_data_##inst, ctx),                               \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(inst), ctx)};                          \
	static const struct spi_rts5817_config spi_rts5817_config_##inst = {                       \
		.ctrl_regs = DT_INST_REG_ADDR(inst),                                               \
		.dw_spi_dev = DEVICE_DT_GET(DW_PHANDLE(inst)),                                     \
		.clock_dev = DEVICE_DT_GET(DW_CLOCKS_CTLR(inst)),                                  \
		.clock_subsys = (clock_control_subsys_t)DW_CLOCKS_CELL(inst, clkid),               \
		SPI_RTS5817_IRQ_HANDLER_FUNC(inst)};                                               \
	DEVICE_DT_INST_DEFINE(inst, spi_rts5817_init, NULL, &spi_rts5817_data_##inst,              \
			      &spi_rts5817_config_##inst, POST_KERNEL,                             \
			      CONFIG_SPI_RTS5817_INIT_PRIORITY, &rts5817_spi_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_RTS5817_INIT)
