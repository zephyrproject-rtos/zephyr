/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/atomic.h>

LOG_MODULE_REGISTER(nxp_4ch_dma, CONFIG_DMA_LOG_LEVEL);

#define DT_DRV_COMPAT	nxp_4ch_dma

struct nxp_dma_chan_data {
	const struct device *dev;
	dma_callback_t cb;
	void *user_data;
	uint32_t width;
	uint8_t src_inc;
	uint8_t dst_inc;
	enum dma_channel_direction dir;
	bool busy;
	uint16_t dmamux_source;
};

struct nxp_dma_data {
	struct dma_context ctx;
	struct nxp_dma_chan_data *chan;
};

struct nxp_dma_config {
	DMA_Type *dma_base;
	DMAMUX_Type *dmamux_base;
	uint8_t num_channels;
	void (*irq_config_func)(const struct device *dev);
	const struct device *dma_clk_dev;
	clock_control_subsys_t dma_clk_subsys;
	const struct device *dmamux_clk_dev;
	clock_control_subsys_t dmamux_clk_subsys;
};

static inline uint32_t nxp_dma_bytes_to_size_field(uint32_t bytes)
{
	switch (bytes) {
	case 1:
		return 1U; /* 8 bits */
	case 2:
		return 2U; /* 16 bits */
	case 4:
		return 0U; /* 32 bits */
	default:
		return 0U; /* default to 32 bits */
	}
}

static inline void nxp_dma_reset_channel(DMA_Type *dma, uint32_t ch)
{
	/* Clear DSR_BCR[DONE] and reset registers to defaults */
	dma->DMA[ch].DSR_BCR |= DMA_DSR_BCR_DONE_MASK;
	dma->DMA[ch].SAR = 0U;
	dma->DMA[ch].DAR = 0U;
	dma->DMA[ch].DSR_BCR = 0U;
	/* Enable auto stop request and cycle steal by default */
	dma->DMA[ch].DCR = DMA_DCR_D_REQ_MASK | DMA_DCR_CS_MASK;
}

static int nxp_dma_configure(const struct device *dev, uint32_t channel,
			struct dma_config *config)
{
	const struct nxp_dma_config *cfg = dev->config;
	struct nxp_dma_data *data = dev->data;
	struct nxp_dma_chan_data *chan_data = &data->chan[channel];
	DMAMUX_Type *mux = cfg->dmamux_base;
	DMA_Type *dma = cfg->dma_base;
	uint8_t src_inc, dst_inc;

	if (channel >= cfg->num_channels || config == NULL ||
		config->head_block == NULL) {
		return -EINVAL;
	}

	if (!((config->source_data_size == 1U) || (config->source_data_size == 2U) ||
		(config->source_data_size == 4U))) {
		return -EINVAL;
	}

	if (config->dest_data_size != config->source_data_size) {
		return -EINVAL;
	}

	/* Reset channel. */
	nxp_dma_reset_channel(dma, channel);

	/* Source and destination address increment. */
	src_inc = (config->head_block->source_addr_adj == DMA_ADDR_ADJ_NO_CHANGE) ? 0U : 1U;
	dst_inc = (config->head_block->dest_addr_adj == DMA_ADDR_ADJ_NO_CHANGE) ? 0U : 1U;

	/* Save channel settings */
	chan_data->dev = dev;
	chan_data->busy = false;
	chan_data->src_inc = src_inc;
	chan_data->dst_inc = dst_inc;
	chan_data->cb = config->dma_callback;
	chan_data->user_data = config->user_data;
	chan_data->width = config->dest_data_size;
	chan_data->dir = config->channel_direction;
	chan_data->dmamux_source = (uint16_t)config->dma_slot;

	/* Configure DCR register:
	 *     destination size, source size, destination increment, source increment.
	 */
	dma->DMA[channel].DCR &= ~(DMA_DCR_DSIZE_MASK | DMA_DCR_SSIZE_MASK |
			DMA_DCR_DINC_MASK | DMA_DCR_SINC_MASK |
			DMA_DCR_EINT_MASK | DMA_DCR_ERQ_MASK);

	dma->DMA[channel].DCR |= (DMA_DCR_DSIZE(nxp_dma_bytes_to_size_field(chan_data->width)) |
			DMA_DCR_SSIZE(nxp_dma_bytes_to_size_field(chan_data->width)) |
			DMA_DCR_DINC((uint32_t)chan_data->dst_inc) |
			DMA_DCR_SINC((uint32_t)chan_data->src_inc));

	/* Trigger an interrupt after transmission is complete. */
	if (config->complete_callback_en || chan_data->cb) {
		dma->DMA[channel].DCR |= DMA_DCR_EINT_MASK;
	}

	/* Configure DMAMUX if available. */
	if (mux != NULL) {
		if (config->dma_slot != 0U) {
			mux->CHCFG[channel] =
				(mux->CHCFG[channel] & (uint8_t)~DMAMUX_CHCFG_SOURCE_MASK) |
				DMAMUX_CHCFG_SOURCE(config->dma_slot);
			mux->CHCFG[channel] |= DMAMUX_CHCFG_ENBL_MASK;
		} else {
			mux->CHCFG[channel] &= (uint8_t)~DMAMUX_CHCFG_ENBL_MASK;
		}
	}

	const struct dma_block_config *blk = config->head_block;

	/* Enforce alignment to width */
	if (((blk->source_address % chan_data->width) != 0U) ||
		((blk->dest_address % chan_data->width) != 0U)) {
		return -EINVAL;
	}

	dma->DMA[channel].SAR = blk->source_address;
	dma->DMA[channel].DAR = blk->dest_address;
	dma->DMA[channel].DSR_BCR = DMA_DSR_BCR_BCR(blk->block_size);

	return 0;
}

/**
 * @note: start() does not reprogram the SAR/DAR/BCR registers, but instead
 * directly uses the values stored in the current hardware registers.
 * These values originate from the most recent configure()/reload() call,
 * or from the register state after the last transmission (including any
 * remaining BCR settings preserved by stop() to support repeated starts,
 * and any advanced SAR/DAR registers).
 */
static int nxp_dma_start(const struct device *dev, uint32_t channel)
{
	const struct nxp_dma_config *cfg = dev->config;
	struct nxp_dma_data *data = dev->data;
	struct nxp_dma_chan_data *chan_data = &data->chan[channel];
	DMAMUX_Type *mux = cfg->dmamux_base;
	DMA_Type *dma = cfg->dma_base;

	if (channel >= cfg->num_channels) {
		return -EINVAL;
	}

	if (chan_data->busy) {
		return -EBUSY;
	}

	if ((dma->DMA[channel].DSR_BCR & DMA_DSR_BCR_BCR_MASK) == 0U) {
		return -EINVAL;
	}

	chan_data->busy = true;

	/* If DMAMUX is enabled, then enable hardware trigger.
	 * Otherwise, enable software trigger.
	 */
	if ((mux != NULL) && (chan_data->dmamux_source != 0U)) {
		dma->DMA[channel].DCR |= (DMA_DCR_ERQ_MASK | DMA_DCR_CS_MASK);
		dma->DMA[channel].DCR &= ~DMA_DCR_START_MASK;
	} else {
		dma->DMA[channel].DCR &= ~(DMA_DCR_ERQ_MASK | DMA_DCR_CS_MASK);
		dma->DMA[channel].DCR |= DMA_DCR_START_MASK;
	}

	return 0;
}

static int nxp_dma_stop(const struct device *dev, uint32_t channel)
{
	const struct nxp_dma_config *cfg = dev->config;
	struct nxp_dma_data *data = dev->data;
	struct nxp_dma_chan_data *chan_data = &data->chan[channel];
	DMA_Type *dma = cfg->dma_base;

	if (channel >= cfg->num_channels) {
		return -EINVAL;
	}

	/* Disable ERQ to stop further HW requests */
	dma->DMA[channel].DCR &= ~DMA_DCR_ERQ_MASK;

	/* Disable DMAMUX channel. */
	if (cfg->dmamux_base) {
		cfg->dmamux_base->CHCFG[channel] &= (uint8_t)~DMAMUX_CHCFG_ENBL_MASK;
	}

	/* Capture remaining BCR and then restore the remaining BCR to enable
	 * subsequent continuation via start() without calling configure()/reload().
	 * Note that SAR/DAR have advanced to the current position during transmission
	 * and are intentionally not restored to enable 'resumable download'.
	 */
	uint32_t remain = (dma->DMA[channel].DSR_BCR & DMA_DSR_BCR_BCR_MASK) >>
				DMA_DSR_BCR_BCR_SHIFT;

	/* Clear status/error bits; note this also clears BCR to 0 per RM */
	dma->DMA[channel].DSR_BCR |= DMA_DSR_BCR_DONE_MASK;

	if (remain != 0U) {
		dma->DMA[channel].DSR_BCR = DMA_DSR_BCR_BCR(remain);
	}

	chan_data->busy = false;

	return 0;
}

static int nxp_dma_reload(const struct device *dev, uint32_t channel,
			uint32_t src, uint32_t dst, size_t size)
{
	const struct nxp_dma_config *cfg = dev->config;
	struct nxp_dma_data *data = dev->data;
	DMA_Type *dma = cfg->dma_base;
	struct nxp_dma_chan_data *chan_data = &data->chan[channel];

	if (channel >= cfg->num_channels) {
		return -EINVAL;
	}

	/* Alignment requirements: address aligned to transfer width */
	if ((src % chan_data->width) != 0U || (dst % chan_data->width) != 0U) {
		return -EINVAL;
	}

	if (chan_data->busy) {
		return -EBUSY;
	}

	/* Configure SAR/DAR and BCR */
	dma->DMA[channel].SAR = src;
	dma->DMA[channel].DAR = dst;
	dma->DMA[channel].DSR_BCR = DMA_DSR_BCR_BCR(size);

	return 0;
}

static int nxp_dma_get_status(const struct device *dev, uint32_t channel,
			struct dma_status *status)
{
	const struct nxp_dma_config *cfg = dev->config;
	struct nxp_dma_data *data = dev->data;
	DMA_Type *dma = cfg->dma_base;
	struct nxp_dma_chan_data *chan_data = &data->chan[channel];

	if ((channel >= cfg->num_channels) || (status == NULL)) {
		return -EINVAL;
	}

	status->busy = ((dma->DMA[channel].DSR_BCR & DMA_DSR_BCR_BSY_MASK) != 0U) &&
				chan_data->busy;
	status->pending_length = (dma->DMA[channel].DSR_BCR & DMA_DSR_BCR_BCR_MASK) >>
					DMA_DSR_BCR_BCR_SHIFT;
	status->dir = chan_data->dir;

	return 0;
}

static void nxp_dma_isr(const struct device *dev, uint32_t channel)
{
	const struct nxp_dma_config *cfg = dev->config;
	struct nxp_dma_data *data = dev->data;
	struct nxp_dma_chan_data *chan_data = &data->chan[channel];
	DMA_Type *dma = cfg->dma_base;
	int ret = DMA_STATUS_COMPLETE;

	if (dma->DMA[channel].DSR_BCR & (DMA_DSR_BCR_BED_MASK |
		DMA_DSR_BCR_BES_MASK | DMA_DSR_BCR_CE_MASK)) {
		ret = -EIO;
	}

	/* Clear DONE flags */
	dma->DMA[channel].DSR_BCR |= DMA_DSR_BCR_DONE_MASK;

	chan_data->busy = false;

	if (chan_data->cb) {
		chan_data->cb(dev, chan_data->user_data, channel, ret);
	}

	barrier_dsync_fence_full();
}

static int nxp_dma_init(const struct device *dev)
{
	const struct nxp_dma_config *cfg = dev->config;
	struct nxp_dma_data *data = dev->data;
	int ret;

	if (cfg->dma_clk_dev != NULL) {
		if (!device_is_ready(cfg->dma_clk_dev)) {
			LOG_ERR("DMA clock device not ready");
			return -ENODEV;
		}
		ret = clock_control_on(cfg->dma_clk_dev, cfg->dma_clk_subsys);
		if (ret < 0) {
			LOG_ERR("Failed to enable DMA clock (%d)", ret);
			return ret;
		}
	}

	if (cfg->dmamux_clk_dev != NULL) {
		if (!device_is_ready(cfg->dmamux_clk_dev)) {
			LOG_ERR("DMAMUX clock device not ready");
			return -ENODEV;
		}
		ret = clock_control_on(cfg->dmamux_clk_dev, cfg->dmamux_clk_subsys);
		if (ret < 0) {
			LOG_ERR("Failed to enable DMAMUX clock (%d)", ret);
			return ret;
		}
	}

	/* Reset all channels */
	for (uint32_t ch = 0; ch < cfg->num_channels; ch++) {
		nxp_dma_reset_channel(cfg->dma_base, ch);

		/* Disable DMAMUX channel if present */
		if (cfg->dmamux_base) {
			cfg->dmamux_base->CHCFG[ch] &= (uint8_t)~DMAMUX_CHCFG_ENBL_MASK;
		}

		data->chan[ch].busy = false;
		data->chan[ch].cb = NULL;
		data->chan[ch].user_data = NULL;
	}

	cfg->irq_config_func(dev);

	return 0;
}

static DEVICE_API(dma, nxp_dma_api) = {
	.config = nxp_dma_configure,
	.start = nxp_dma_start,
	.stop = nxp_dma_stop,
	.reload = nxp_dma_reload,
	.get_status = nxp_dma_get_status,
};

/* IRQ dispatcher per-channel */
#define NXP_DMA_DECLARE_IRQ(inst, ch)							\
	static void _CONCAT(_CONCAT(nxp_dma_irq, inst), ch)(const struct device *dev)	\
	{										\
		nxp_dma_isr(dev, ch);							\
	}

/* Per-instance macros, connect each channel IRQ by index. */
#define NXP_DMA_IRQ_CFG_FUNC(inst)							\
	static void _CONCAT(nxp_dma_irq_config_func, inst)(const struct device *dev)	\
	{										\
		IF_ENABLED(DT_INST_IRQ_HAS_IDX(inst, 0), (				\
			IRQ_CONNECT(DT_INST_IRQ_BY_IDX(inst, 0, irq),			\
				DT_INST_IRQ_BY_IDX(inst, 0, priority),			\
				_CONCAT(_CONCAT(nxp_dma_irq, inst), 0),			\
				DEVICE_DT_INST_GET(inst), 0);				\
			irq_enable(DT_INST_IRQ_BY_IDX(inst, 0, irq));			\
		))									\
		IF_ENABLED(DT_INST_IRQ_HAS_IDX(inst, 1), (				\
			IRQ_CONNECT(DT_INST_IRQ_BY_IDX(inst, 1, irq),			\
				DT_INST_IRQ_BY_IDX(inst, 1, priority),			\
				_CONCAT(_CONCAT(nxp_dma_irq, inst), 1),			\
				DEVICE_DT_INST_GET(inst), 0);				\
			irq_enable(DT_INST_IRQ_BY_IDX(inst, 1, irq));			\
		))									\
		IF_ENABLED(DT_INST_IRQ_HAS_IDX(inst, 2), (				\
			IRQ_CONNECT(DT_INST_IRQ_BY_IDX(inst, 2, irq),			\
				DT_INST_IRQ_BY_IDX(inst, 2, priority),			\
				_CONCAT(_CONCAT(nxp_dma_irq, inst), 2),			\
				DEVICE_DT_INST_GET(inst), 0);				\
			irq_enable(DT_INST_IRQ_BY_IDX(inst, 2, irq));			\
		))									\
		IF_ENABLED(DT_INST_IRQ_HAS_IDX(inst, 3), (				\
			IRQ_CONNECT(DT_INST_IRQ_BY_IDX(inst, 3, irq),			\
				DT_INST_IRQ_BY_IDX(inst, 3, priority),			\
				_CONCAT(_CONCAT(nxp_dma_irq, inst), 3),			\
				DEVICE_DT_INST_GET(inst), 0);				\
			irq_enable(DT_INST_IRQ_BY_IDX(inst, 3, irq));			\
		));									\
	}

#define NXP_DMA_INIT(inst)									\
	NXP_DMA_DECLARE_IRQ(inst, 0)								\
	NXP_DMA_DECLARE_IRQ(inst, 1)								\
	NXP_DMA_DECLARE_IRQ(inst, 2)								\
	NXP_DMA_DECLARE_IRQ(inst, 3)								\
	NXP_DMA_IRQ_CFG_FUNC(inst)								\
												\
	ATOMIC_DEFINE(_CONCAT(nxp_dma_atomic, inst), DT_INST_PROP(inst, dma_channels));		\
												\
	static struct nxp_dma_chan_data								\
		_CONCAT(nxp_dma_chan_data, inst)[DT_INST_PROP(inst, dma_channels)];		\
												\
	static struct nxp_dma_data _CONCAT(nxp_dma_runtime, inst) = {				\
		.ctx = {									\
			.magic = DMA_MAGIC,							\
			.dma_channels = DT_INST_PROP(inst, dma_channels),			\
			.atomic = _CONCAT(nxp_dma_atomic, inst),				\
		},										\
		.chan = _CONCAT(nxp_dma_chan_data, inst),					\
	};											\
												\
	static const struct nxp_dma_config _CONCAT(nxp_dma_config, inst) = {			\
		.dma_base = (DMA_Type *)DT_INST_REG_ADDR_BY_IDX(inst, 0),			\
		.dmamux_base = (DMAMUX_Type *)DT_INST_REG_ADDR_BY_IDX(inst, 1),			\
		.num_channels = DT_INST_PROP(inst, dma_channels),				\
		.irq_config_func = _CONCAT(nxp_dma_irq_config_func, inst),			\
		.dma_clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_IDX(inst, 0)),		\
		.dma_clk_subsys =								\
			(clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_IDX(inst, 0, name),	\
		.dmamux_clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_IDX(inst, 1)),		\
		.dmamux_clk_subsys =								\
			(clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_IDX(inst, 1, name),	\
	};											\
												\
	DEVICE_DT_INST_DEFINE(inst, nxp_dma_init, NULL, &_CONCAT(nxp_dma_runtime, inst),	\
			&_CONCAT(nxp_dma_config, inst), PRE_KERNEL_1,				\
			CONFIG_DMA_INIT_PRIORITY, &nxp_dma_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_DMA_INIT)
