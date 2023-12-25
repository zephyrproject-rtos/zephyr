/*
 * Copyright (c) 2021 BrainCo Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gd_gd32_spi

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/gd32.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/spi.h>
#ifdef CONFIG_SPI_GD32_DMA
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_gd32.h>
#endif

#include <gd32_spi.h>

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(spi_gd32);

#include "spi_context.h"

/* SPI error status mask. */
#define SPI_GD32_ERR_MASK	(SPI_STAT_RXORERR | SPI_STAT_CONFERR | SPI_STAT_CRCERR)

#define GD32_SPI_PSC_MAX	0x7U

#ifdef CONFIG_SPI_GD32_DMA

enum spi_gd32_dma_direction {
	RX = 0,
	TX,
	NUM_OF_DIRECTION
};

struct spi_gd32_dma_config {
	const struct device *dev;
	uint32_t channel;
	uint32_t config;
	uint32_t slot;
	uint32_t fifo_threshold;
};

struct spi_gd32_dma_data {
	struct dma_config config;
	struct dma_block_config block;
	uint32_t count;
};

#endif

struct spi_gd32_config {
	uint32_t reg;
	uint16_t clkid;
	struct reset_dt_spec reset;
	const struct pinctrl_dev_config *pcfg;
#ifdef CONFIG_SPI_GD32_DMA
	const struct spi_gd32_dma_config dma[NUM_OF_DIRECTION];
#endif
#ifdef CONFIG_SPI_GD32_INTERRUPT
	void (*irq_configure)();
#endif
};

struct spi_gd32_data {
	struct spi_context ctx;
#ifdef CONFIG_SPI_GD32_DMA
	struct spi_gd32_dma_data dma[NUM_OF_DIRECTION];
#endif
};

#ifdef CONFIG_SPI_GD32_DMA

static uint32_t dummy_tx;
static uint32_t dummy_rx;

static bool spi_gd32_dma_enabled(const struct device *dev)
{
	const struct spi_gd32_config *cfg = dev->config;

	if (cfg->dma[TX].dev && cfg->dma[RX].dev) {
		return true;
	}

	return false;
}

static size_t spi_gd32_dma_enabled_num(const struct device *dev)
{
	return spi_gd32_dma_enabled(dev) ? 2 : 0;
}

#endif

static int spi_gd32_get_err(const struct spi_gd32_config *cfg)
{
	uint32_t stat = SPI_STAT(cfg->reg);

	if (stat & SPI_GD32_ERR_MASK) {
		LOG_ERR("spi%u error status detected, err = %u",
			cfg->reg, stat & (uint32_t)SPI_GD32_ERR_MASK);

		return -EIO;
	}

	return 0;
}

static bool spi_gd32_transfer_ongoing(struct spi_gd32_data *data)
{
	return spi_context_tx_on(&data->ctx) ||
	       spi_context_rx_on(&data->ctx);
}

static int spi_gd32_configure(const struct device *dev,
			      const struct spi_config *config)
{
	struct spi_gd32_data *data = dev->data;
	const struct spi_gd32_config *cfg = dev->config;
	uint32_t bus_freq;

	if (spi_context_configured(&data->ctx, config)) {
		return 0;
	}

	if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_SLAVE) {
		LOG_ERR("Slave mode not supported");
		return -ENOTSUP;
	}

	SPI_CTL0(cfg->reg) &= ~SPI_CTL0_SPIEN;

	SPI_CTL0(cfg->reg) |= SPI_MASTER;
	SPI_CTL0(cfg->reg) &= ~SPI_TRANSMODE_BDTRANSMIT;

	if (SPI_WORD_SIZE_GET(config->operation) == 8) {
		SPI_CTL0(cfg->reg) |= SPI_FRAMESIZE_8BIT;
	} else {
		SPI_CTL0(cfg->reg) |= SPI_FRAMESIZE_16BIT;
	}

	/* Reset to hardware NSS mode. */
	SPI_CTL0(cfg->reg) &= ~SPI_CTL0_SWNSSEN;
	if (spi_cs_is_gpio(config)) {
		SPI_CTL0(cfg->reg) |= SPI_CTL0_SWNSSEN;
	} else {
		/*
		 * For single master env,
		 * hardware NSS mode also need to set the NSSDRV bit.
		 */
		SPI_CTL1(cfg->reg) |= SPI_CTL1_NSSDRV;
	}

	SPI_CTL0(cfg->reg) &= ~SPI_CTL0_LF;
	if (config->operation & SPI_TRANSFER_LSB) {
		SPI_CTL0(cfg->reg) |= SPI_CTL0_LF;
	}

	SPI_CTL0(cfg->reg) &= ~SPI_CTL0_CKPL;
	if (config->operation & SPI_MODE_CPOL) {
		SPI_CTL0(cfg->reg) |= SPI_CTL0_CKPL;
	}

	SPI_CTL0(cfg->reg) &= ~SPI_CTL0_CKPH;
	if (config->operation & SPI_MODE_CPHA) {
		SPI_CTL0(cfg->reg) |= SPI_CTL0_CKPH;
	}

	(void)clock_control_get_rate(GD32_CLOCK_CONTROLLER,
				     (clock_control_subsys_t)&cfg->clkid,
				     &bus_freq);

	for (uint8_t i = 0U; i <= GD32_SPI_PSC_MAX; i++) {
		bus_freq = bus_freq >> 1U;
		if (bus_freq <= config->frequency) {
			SPI_CTL0(cfg->reg) &= ~SPI_CTL0_PSC;
			SPI_CTL0(cfg->reg) |= CTL0_PSC(i);
			break;
		}
	}

	data->ctx.config = config;

	return 0;
}

static int spi_gd32_frame_exchange(const struct device *dev)
{
	struct spi_gd32_data *data = dev->data;
	const struct spi_gd32_config *cfg = dev->config;
	struct spi_context *ctx = &data->ctx;
	uint16_t tx_frame = 0U, rx_frame = 0U;

	while ((SPI_STAT(cfg->reg) & SPI_STAT_TBE) == 0) {
		/* NOP */
	}

	if (SPI_WORD_SIZE_GET(ctx->config->operation) == 8) {
		if (spi_context_tx_buf_on(ctx)) {
			tx_frame = UNALIGNED_GET((uint8_t *)(data->ctx.tx_buf));
		}
		/* For 8 bits mode, spi will forced SPI_DATA[15:8] to 0. */
		SPI_DATA(cfg->reg) = tx_frame;

		spi_context_update_tx(ctx, 1, 1);
	} else {
		if (spi_context_tx_buf_on(ctx)) {
			tx_frame = UNALIGNED_GET((uint8_t *)(data->ctx.tx_buf));
		}
		SPI_DATA(cfg->reg) = tx_frame;

		spi_context_update_tx(ctx, 2, 1);
	}

	while ((SPI_STAT(cfg->reg) & SPI_STAT_RBNE) == 0) {
		/* NOP */
	}

	if (SPI_WORD_SIZE_GET(data->ctx.config->operation) == 8) {
		/* For 8 bits mode, spi will forced SPI_DATA[15:8] to 0. */
		rx_frame = SPI_DATA(cfg->reg);
		if (spi_context_rx_buf_on(ctx)) {
			UNALIGNED_PUT(rx_frame, (uint8_t *)data->ctx.rx_buf);
		}

		spi_context_update_rx(ctx, 1, 1);
	} else {
		rx_frame = SPI_DATA(cfg->reg);
		if (spi_context_rx_buf_on(ctx)) {
			UNALIGNED_PUT(rx_frame, (uint16_t *)data->ctx.rx_buf);
		}

		spi_context_update_rx(ctx, 2, 1);
	}

	return spi_gd32_get_err(cfg);
}

#ifdef CONFIG_SPI_GD32_DMA
static void spi_gd32_dma_callback(const struct device *dma_dev, void *arg,
				  uint32_t channel, int status);

static uint32_t spi_gd32_dma_setup(const struct device *dev, const uint32_t dir)
{
	const struct spi_gd32_config *cfg = dev->config;
	struct spi_gd32_data *data = dev->data;
	struct dma_config *dma_cfg = &data->dma[dir].config;
	struct dma_block_config *block_cfg = &data->dma[dir].block;
	const struct spi_gd32_dma_config *dma = &cfg->dma[dir];
	int ret;

	memset(dma_cfg, 0, sizeof(struct dma_config));
	memset(block_cfg, 0, sizeof(struct dma_block_config));

	dma_cfg->source_burst_length = 1;
	dma_cfg->dest_burst_length = 1;
	dma_cfg->user_data = (void *)dev;
	dma_cfg->dma_callback = spi_gd32_dma_callback;
	dma_cfg->block_count = 1U;
	dma_cfg->head_block = block_cfg;
	dma_cfg->dma_slot = cfg->dma[dir].slot;
	dma_cfg->channel_priority =
		GD32_DMA_CONFIG_PRIORITY(cfg->dma[dir].config);
	dma_cfg->channel_direction =
		dir == TX ? MEMORY_TO_PERIPHERAL : PERIPHERAL_TO_MEMORY;

	if (SPI_WORD_SIZE_GET(data->ctx.config->operation) == 8) {
		dma_cfg->source_data_size = 1;
		dma_cfg->dest_data_size = 1;
	} else {
		dma_cfg->source_data_size = 2;
		dma_cfg->dest_data_size = 2;
	}

	block_cfg->block_size = spi_context_max_continuous_chunk(&data->ctx);

	if (dir == TX) {
		block_cfg->dest_address = (uint32_t)&SPI_DATA(cfg->reg);
		block_cfg->dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		if (spi_context_tx_buf_on(&data->ctx)) {
			block_cfg->source_address = (uint32_t)data->ctx.tx_buf;
			block_cfg->source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		} else {
			block_cfg->source_address = (uint32_t)&dummy_tx;
			block_cfg->source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		}
	}

	if (dir == RX) {
		block_cfg->source_address = (uint32_t)&SPI_DATA(cfg->reg);
		block_cfg->source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;

		if (spi_context_rx_buf_on(&data->ctx)) {
			block_cfg->dest_address = (uint32_t)data->ctx.rx_buf;
			block_cfg->dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		} else {
			block_cfg->dest_address = (uint32_t)&dummy_rx;
			block_cfg->dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		}
	}

	ret = dma_config(dma->dev, dma->channel, dma_cfg);
	if (ret < 0) {
		LOG_ERR("dma_config %p failed %d\n", dma->dev, ret);
		return ret;
	}

	ret = dma_start(dma->dev, dma->channel);
	if (ret < 0) {
		LOG_ERR("dma_start %p failed %d\n", dma->dev, ret);
		return ret;
	}

	return 0;
}

static int spi_gd32_start_dma_transceive(const struct device *dev)
{
	const struct spi_gd32_config *cfg = dev->config;
	struct spi_gd32_data *data = dev->data;
	const size_t chunk_len = spi_context_max_continuous_chunk(&data->ctx);
	struct dma_status stat;
	int ret = 0;

	for (size_t i = 0; i < spi_gd32_dma_enabled_num(dev); i++) {
		dma_get_status(cfg->dma[i].dev, cfg->dma[i].channel, &stat);
		if ((chunk_len != data->dma[i].count) && !stat.busy) {
			ret = spi_gd32_dma_setup(dev, i);
			if (ret < 0) {
				goto on_error;
			}
		}
	}

	SPI_CTL1(cfg->reg) |= (SPI_CTL1_DMATEN | SPI_CTL1_DMAREN);

on_error:
	if (ret < 0) {
		for (size_t i = 0; i < spi_gd32_dma_enabled_num(dev); i++) {
			dma_stop(cfg->dma[i].dev, cfg->dma[i].channel);
		}
	}
	return ret;
}
#endif

static int spi_gd32_transceive_impl(const struct device *dev,
				    const struct spi_config *config,
				    const struct spi_buf_set *tx_bufs,
				    const struct spi_buf_set *rx_bufs,
				    spi_callback_t cb,
				    void *userdata)
{
	struct spi_gd32_data *data = dev->data;
	const struct spi_gd32_config *cfg = dev->config;
	int ret;

	spi_context_lock(&data->ctx, (cb != NULL), cb, userdata, config);

	ret = spi_gd32_configure(dev, config);
	if (ret < 0) {
		goto error;
	}

	SPI_CTL0(cfg->reg) |= SPI_CTL0_SPIEN;

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

	spi_context_cs_control(&data->ctx, true);

#ifdef CONFIG_SPI_GD32_INTERRUPT
#ifdef CONFIG_SPI_GD32_DMA
	if (spi_gd32_dma_enabled(dev)) {
		for (size_t i = 0; i < ARRAY_SIZE(data->dma); i++) {
			data->dma[i].count = 0;
		}

		ret = spi_gd32_start_dma_transceive(dev);
		if (ret < 0) {
			goto dma_error;
		}
	} else
#endif
	{
		SPI_STAT(cfg->reg) &=
			~(SPI_STAT_RBNE | SPI_STAT_TBE | SPI_GD32_ERR_MASK);
		SPI_CTL1(cfg->reg) |=
			(SPI_CTL1_RBNEIE | SPI_CTL1_TBEIE | SPI_CTL1_ERRIE);
	}
	ret = spi_context_wait_for_completion(&data->ctx);
#else
	do {
		ret = spi_gd32_frame_exchange(dev);
		if (ret < 0) {
			break;
		}
	} while (spi_gd32_transfer_ongoing(data));

#ifdef CONFIG_SPI_ASYNC
	spi_context_complete(&data->ctx, dev, ret);
#endif
#endif

	while (!(SPI_STAT(cfg->reg) & SPI_STAT_TBE) ||
		(SPI_STAT(cfg->reg) & SPI_STAT_TRANS)) {
		/* Wait until last frame transfer complete. */
	}

#ifdef CONFIG_SPI_GD32_DMA
dma_error:
#endif
	spi_context_cs_control(&data->ctx, false);

	SPI_CTL0(cfg->reg) &=
		~(SPI_CTL0_SPIEN | SPI_CTL1_DMATEN | SPI_CTL1_DMAREN);

error:
	spi_context_release(&data->ctx, ret);

	return ret;
}

static int spi_gd32_transceive(const struct device *dev,
			       const struct spi_config *config,
			       const struct spi_buf_set *tx_bufs,
			       const struct spi_buf_set *rx_bufs)
{
	return spi_gd32_transceive_impl(dev, config, tx_bufs, rx_bufs, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int spi_gd32_transceive_async(const struct device *dev,
				     const struct spi_config *config,
				     const struct spi_buf_set *tx_bufs,
				     const struct spi_buf_set *rx_bufs,
				     spi_callback_t cb,
				     void *userdata)
{
	return spi_gd32_transceive_impl(dev, config, tx_bufs, rx_bufs, cb, userdata);
}
#endif

#ifdef CONFIG_SPI_GD32_INTERRUPT

static void spi_gd32_complete(const struct device *dev, int status)
{
	struct spi_gd32_data *data = dev->data;
	const struct spi_gd32_config *cfg = dev->config;

	SPI_CTL1(cfg->reg) &=
		~(SPI_CTL1_RBNEIE | SPI_CTL1_TBEIE | SPI_CTL1_ERRIE);

#ifdef CONFIG_SPI_GD32_DMA
	for (size_t i = 0; i < spi_gd32_dma_enabled_num(dev); i++) {
		dma_stop(cfg->dma[i].dev, cfg->dma[i].channel);
	}
#endif

	spi_context_complete(&data->ctx, dev, status);
}

static void spi_gd32_isr(struct device *dev)
{
	const struct spi_gd32_config *cfg = dev->config;
	struct spi_gd32_data *data = dev->data;
	int err = 0;

	err = spi_gd32_get_err(cfg);
	if (err) {
		spi_gd32_complete(dev, err);
		return;
	}

	if (spi_gd32_transfer_ongoing(data)) {
		err = spi_gd32_frame_exchange(dev);
	}

	if (err || !spi_gd32_transfer_ongoing(data)) {
		spi_gd32_complete(dev, err);
	}
}

#endif /* SPI_GD32_INTERRUPT */

#ifdef CONFIG_SPI_GD32_DMA

static bool spi_gd32_chunk_transfer_finished(const struct device *dev)
{
	struct spi_gd32_data *data = dev->data;
	struct spi_gd32_dma_data *dma = data->dma;
	const size_t chunk_len = spi_context_max_continuous_chunk(&data->ctx);

	return (MIN(dma[TX].count, dma[RX].count) >= chunk_len);
}

static void spi_gd32_dma_callback(const struct device *dma_dev, void *arg,
				  uint32_t channel, int status)
{
	const struct device *dev = (const struct device *)arg;
	const struct spi_gd32_config *cfg = dev->config;
	struct spi_gd32_data *data = dev->data;
	const size_t chunk_len = spi_context_max_continuous_chunk(&data->ctx);
	int err = 0;

	if (status < 0) {
		LOG_ERR("dma:%p ch:%d callback gets error: %d", dma_dev, channel,
			status);
		spi_gd32_complete(dev, status);
		return;
	}

	for (size_t i = 0; i < ARRAY_SIZE(cfg->dma); i++) {
		if (dma_dev == cfg->dma[i].dev &&
		    channel == cfg->dma[i].channel) {
			data->dma[i].count += chunk_len;
		}
	}

	/* Check transfer finished.
	 * The transmission of this chunk is complete if both the dma[TX].count
	 * and the dma[RX].count reach greater than or equal to the chunk_len.
	 * chunk_len is zero here means the transfer is already complete.
	 */
	if (spi_gd32_chunk_transfer_finished(dev)) {
		if (SPI_WORD_SIZE_GET(data->ctx.config->operation) == 8) {
			spi_context_update_tx(&data->ctx, 1, chunk_len);
			spi_context_update_rx(&data->ctx, 1, chunk_len);
		} else {
			spi_context_update_tx(&data->ctx, 2, chunk_len);
			spi_context_update_rx(&data->ctx, 2, chunk_len);
		}

		if (spi_gd32_transfer_ongoing(data)) {
			/* Next chunk is available, reset the count and
			 * continue processing
			 */
			data->dma[TX].count = 0;
			data->dma[RX].count = 0;
		} else {
			/* All data is processed, complete the process */
			spi_context_complete(&data->ctx, dev, 0);
			return;
		}
	}

	err = spi_gd32_start_dma_transceive(dev);
	if (err) {
		spi_gd32_complete(dev, err);
	}
}

#endif /* DMA */

static int spi_gd32_release(const struct device *dev,
			    const struct spi_config *config)
{
	struct spi_gd32_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static const struct spi_driver_api spi_gd32_driver_api = {
	.transceive = spi_gd32_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_gd32_transceive_async,
#endif
	.release = spi_gd32_release
};

int spi_gd32_init(const struct device *dev)
{
	struct spi_gd32_data *data = dev->data;
	const struct spi_gd32_config *cfg = dev->config;
	int ret;
#ifdef CONFIG_SPI_GD32_DMA
	uint32_t ch_filter;
#endif

	(void)clock_control_on(GD32_CLOCK_CONTROLLER,
			       (clock_control_subsys_t)&cfg->clkid);

	(void)reset_line_toggle_dt(&cfg->reset);

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		LOG_ERR("Failed to apply pinctrl state");
		return ret;
	}

#ifdef CONFIG_SPI_GD32_DMA
	if ((cfg->dma[RX].dev && !cfg->dma[TX].dev) ||
	    (cfg->dma[TX].dev && !cfg->dma[RX].dev)) {
		LOG_ERR("DMA must be enabled for both TX and RX channels");
		return -ENODEV;
	}

	for (size_t i = 0; i < spi_gd32_dma_enabled_num(dev); i++) {
		if (!device_is_ready(cfg->dma[i].dev)) {
			LOG_ERR("DMA %s not ready", cfg->dma[i].dev->name);
			return -ENODEV;
		}

		ch_filter = BIT(cfg->dma[i].channel);
		ret = dma_request_channel(cfg->dma[i].dev, &ch_filter);
		if (ret < 0) {
			LOG_ERR("dma_request_channel failed %d", ret);
			return ret;
		}
	}
#endif

	ret = spi_context_cs_configure_all(&data->ctx);
	if (ret < 0) {
		return ret;
	}

#ifdef CONFIG_SPI_GD32_INTERRUPT
	cfg->irq_configure(dev);
#endif

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#define DMA_INITIALIZER(idx, dir)                                              \
	{                                                                      \
		.dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(idx, dir)),     \
		.channel = DT_INST_DMAS_CELL_BY_NAME(idx, dir, channel),       \
		.slot = COND_CODE_1(                                           \
			DT_HAS_COMPAT_STATUS_OKAY(gd_gd32_dma_v1),             \
			(DT_INST_DMAS_CELL_BY_NAME(idx, dir, slot)), (0)),     \
		.config = DT_INST_DMAS_CELL_BY_NAME(idx, dir, config),         \
		.fifo_threshold = COND_CODE_1(                                 \
			DT_HAS_COMPAT_STATUS_OKAY(gd_gd32_dma_v1),             \
			(DT_INST_DMAS_CELL_BY_NAME(idx, dir, fifo_threshold)), \
			(0)),						  \
	}

#define DMAS_DECL(idx)                                                         \
	{                                                                      \
		COND_CODE_1(DT_INST_DMAS_HAS_NAME(idx, rx),                    \
			    (DMA_INITIALIZER(idx, rx)), ({0})),                \
		COND_CODE_1(DT_INST_DMAS_HAS_NAME(idx, tx),                    \
			    (DMA_INITIALIZER(idx, tx)), ({0})),                \
	}

#define GD32_IRQ_CONFIGURE(idx)						   \
	static void spi_gd32_irq_configure_##idx(void)			   \
	{								   \
		IRQ_CONNECT(DT_INST_IRQN(idx), DT_INST_IRQ(idx, priority), \
			    spi_gd32_isr,				   \
			    DEVICE_DT_INST_GET(idx), 0);		   \
		irq_enable(DT_INST_IRQN(idx));				   \
	}

#define GD32_SPI_INIT(idx)						       \
	PINCTRL_DT_INST_DEFINE(idx);					       \
	IF_ENABLED(CONFIG_SPI_GD32_INTERRUPT, (GD32_IRQ_CONFIGURE(idx)));      \
	static struct spi_gd32_data spi_gd32_data_##idx = {		       \
		SPI_CONTEXT_INIT_LOCK(spi_gd32_data_##idx, ctx),	       \
		SPI_CONTEXT_INIT_SYNC(spi_gd32_data_##idx, ctx),	       \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(idx), ctx) };      \
	static struct spi_gd32_config spi_gd32_config_##idx = {		       \
		.reg = DT_INST_REG_ADDR(idx),				       \
		.clkid = DT_INST_CLOCKS_CELL(idx, id),			       \
		.reset = RESET_DT_SPEC_INST_GET(idx),			       \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),		       \
		IF_ENABLED(CONFIG_SPI_GD32_DMA, (.dma = DMAS_DECL(idx),))      \
		IF_ENABLED(CONFIG_SPI_GD32_INTERRUPT,			       \
			   (.irq_configure = spi_gd32_irq_configure_##idx)) }; \
	DEVICE_DT_INST_DEFINE(idx, &spi_gd32_init, NULL,		       \
			      &spi_gd32_data_##idx, &spi_gd32_config_##idx,    \
			      POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,	       \
			      &spi_gd32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GD32_SPI_INIT)
