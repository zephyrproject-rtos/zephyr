/*
 * Copyright (c) 2023 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/clock_control/adi_max32_clock_control.h>

#include <wrap_max32_dma.h>

#define DT_DRV_COMPAT adi_max32_dma

LOG_MODULE_REGISTER(max32_dma, CONFIG_DMA_LOG_LEVEL);

struct max32_dma_config {
	mxc_dma_regs_t *regs;
	const struct device *clock;
	struct max32_perclk perclk;
	uint8_t channels;
	void (*irq_configure)(void);
};

struct max32_dma_data {
	dma_callback_t callback;
	void *cb_data;
	uint32_t err_cb_dis;
};

static inline bool max32_dma_ch_prio_valid(uint32_t ch_prio)
{
	/* mxc_dma_priority_t is limited to values 0-3 */
	if (!(ch_prio >= 0 && ch_prio <= 3)) {
		LOG_ERR("Invalid DMA priority - must be type mxc_dma_priority_t (0-3)");
		return false;
	}
	return true;
}

static inline int max32_dma_width(uint32_t width)
{
	switch (width) {
	case 1:
		return MXC_DMA_WIDTH_BYTE;
	case 2:
		return MXC_DMA_WIDTH_HALFWORD;
	case 4:
		return MXC_DMA_WIDTH_WORD;
	default:
		LOG_ERR("Invalid DMA width - must be byte (1), halfword (2) or word (4)");
		return -EINVAL;
	}
}

static inline int max32_dma_addr_adj(uint16_t addr_adj)
{
	switch (addr_adj) {
	case DMA_ADDR_ADJ_NO_CHANGE:
		return 0;
	case DMA_ADDR_ADJ_INCREMENT:
		return 1;
	default:
		LOG_ERR("Invalid DMA address adjust - must be NO_CHANGE (0) or INCREMENT (1)");
		return 0;
	}
}

static inline int max32_dma_ch_index(mxc_dma_regs_t *dma, uint8_t ch)
{
	return (ch + MXC_DMA_GET_IDX(dma) * (MXC_DMA_CHANNELS / MXC_DMA_INSTANCES));
}

static int max32_dma_config(const struct device *dev, uint32_t channel, struct dma_config *config)
{
	int ret = 0;
	const struct max32_dma_config *cfg = dev->config;
	struct max32_dma_data *data = dev->data;
	uint32_t ch;

	if (channel >= cfg->channels) {
		LOG_ERR("Invalid DMA channel - must be < %" PRIu32 " (%" PRIu32 ")", cfg->channels,
			channel);
		return -EINVAL;
	}

	ch = max32_dma_ch_index(cfg->regs, channel);

	/* DMA Channel Config */
	mxc_dma_config_t mxc_dma_cfg;

	mxc_dma_cfg.ch = ch;
	mxc_dma_cfg.reqsel = config->dma_slot << ADI_MAX32_DMA_CFG_REQ_POS;
	if (((max32_dma_width(config->source_data_size)) < 0) ||
	    ((max32_dma_width(config->dest_data_size)) < 0)) {
		return -EINVAL;
	}
	mxc_dma_cfg.srcwd = max32_dma_width(config->source_data_size);
	mxc_dma_cfg.dstwd = max32_dma_width(config->dest_data_size);
	mxc_dma_cfg.srcinc_en = max32_dma_addr_adj(config->head_block->source_addr_adj);
	mxc_dma_cfg.dstinc_en = max32_dma_addr_adj(config->head_block->dest_addr_adj);

	/* DMA Channel Advanced Config */
	mxc_dma_adv_config_t mxc_dma_cfg_adv;

	mxc_dma_cfg_adv.ch = ch;
	if (!max32_dma_ch_prio_valid(config->channel_priority)) {
		return -EINVAL;
	}
	mxc_dma_cfg_adv.prio = config->channel_priority;
	mxc_dma_cfg_adv.reqwait_en = 0;
	mxc_dma_cfg_adv.tosel = MXC_DMA_TIMEOUT_4_CLK;
	mxc_dma_cfg_adv.pssel = MXC_DMA_PRESCALE_DISABLE;
	mxc_dma_cfg_adv.burst_size = config->source_burst_length;

	/* DMA Transfer Config */
	mxc_dma_srcdst_t txfer;

	txfer.ch = ch;
	txfer.source = (void *)config->head_block->source_address;
	txfer.dest = (void *)config->head_block->dest_address;
	txfer.len = config->head_block->block_size;

	ret = MXC_DMA_ConfigChannel(mxc_dma_cfg, txfer);
	if (ret != E_NO_ERROR) {
		return ret;
	}

	ret = MXC_DMA_AdvConfigChannel(mxc_dma_cfg_adv);
	if (ret) {
		return ret;
	}

	/* Enable interrupts for the DMA peripheral */
	ret = MXC_DMA_EnableInt(ch);
	if (ret != E_NO_ERROR) {
		return ret;
	}

	/* Enable complete and count-to-zero interrupts for the channel */
	ret = MXC_DMA_ChannelEnableInt(ch, ADI_MAX32_DMA_CTRL_DIS_IE | ADI_MAX32_DMA_CTRL_CTZIEN);
	if (ret != E_NO_ERROR) {
		return ret;
	}

	data[channel].callback = config->dma_callback;
	data[channel].cb_data = config->user_data;
	data[channel].err_cb_dis = config->error_callback_dis;

	return ret;
}

static int max32_dma_reload(const struct device *dev, uint32_t channel, uint32_t src, uint32_t dst,
			    size_t size)
{
	const struct max32_dma_config *cfg = dev->config;
	mxc_dma_srcdst_t reload;
	int flags;

	if (channel >= cfg->channels) {
		LOG_ERR("Invalid DMA channel - must be < %" PRIu32 " (%" PRIu32 ")", cfg->channels,
			channel);
		return -EINVAL;
	}

	channel = max32_dma_ch_index(cfg->regs, channel);
	flags = MXC_DMA_ChannelGetFlags(channel);
	if (flags & ADI_MAX32_DMA_STATUS_ST) {
		return -EBUSY;
	}

	reload.ch = channel;
	reload.source = (void *)src;
	reload.dest = (void *)dst;
	reload.len = size;
	return MXC_DMA_SetSrcDst(reload);
}

static int max32_dma_start(const struct device *dev, uint32_t channel)
{
	const struct max32_dma_config *cfg = dev->config;
	int flags;

	if (channel >= cfg->channels) {
		LOG_ERR("Invalid DMA channel - must be < %" PRIu32 " (%" PRIu32 ")", cfg->channels,
			channel);
		return -EINVAL;
	}

	channel = max32_dma_ch_index(cfg->regs, channel);
	flags = MXC_DMA_ChannelGetFlags(channel);
	if (flags & ADI_MAX32_DMA_STATUS_ST) {
		return -EBUSY;
	}

	return MXC_DMA_Start(channel);
}

static int max32_dma_stop(const struct device *dev, uint32_t channel)
{
	const struct max32_dma_config *cfg = dev->config;

	if (channel >= cfg->channels) {
		LOG_ERR("Invalid DMA channel - must be < %" PRIu32 " (%" PRIu32 ")", cfg->channels,
			channel);
		return -EINVAL;
	}

	channel = max32_dma_ch_index(cfg->regs, channel);

	return MXC_DMA_Stop(channel);
}

static int max32_dma_get_status(const struct device *dev, uint32_t channel, struct dma_status *stat)
{
	const struct max32_dma_config *cfg = dev->config;
	int ret = 0;
	int flags = 0;
	mxc_dma_srcdst_t txfer;

	if (channel >= cfg->channels) {
		LOG_ERR("Invalid DMA channel - must be < %" PRIu32 " (%" PRIu32 ")", cfg->channels,
			channel);
		return -EINVAL;
	}

	channel = max32_dma_ch_index(cfg->regs, channel);
	txfer.ch = channel;

	flags = MXC_DMA_ChannelGetFlags(channel);

	ret = MXC_DMA_GetSrcDst(&txfer);
	if (ret != E_NO_ERROR) {
		return ret;
	}

	/* Channel is busy if channel status is enabled */
	stat->busy = (flags & ADI_MAX32_DMA_STATUS_ST) != 0;
	stat->pending_length = txfer.len;

	return ret;
}

static void max32_dma_isr(const struct device *dev)
{
	const struct max32_dma_config *cfg = dev->config;
	struct max32_dma_data *data = dev->data;
	mxc_dma_regs_t *regs = cfg->regs;
	int ch, c;
	int flags;
	int status = 0;

	uint8_t channel_base = max32_dma_ch_index(cfg->regs, 0);

	for (ch = channel_base, c = 0; c < cfg->channels; ch++, c++) {
		flags = MXC_DMA_ChannelGetFlags(ch);

		/* Check if channel is in use, if not, move to next channel */
		if (flags <= 0) {
			continue;
		}

		/* Check for error interrupts */
		if (flags & (ADI_MAX32_DMA_STATUS_BUS_ERR | ADI_MAX32_DMA_STATUS_TO_IF)) {
			status = -EIO;
		}

		MXC_DMA_ChannelClearFlags(ch, flags);

		if (data[c].callback) {
			/* Only call error callback if enabled during DMA config */
			if (status < 0 && (data[c].err_cb_dis)) {
				break;
			}
			data[c].callback(dev, data[c].cb_data, c, status);
		}

		/* No need to check rest of the channels if no interrupt flags set */
		if (MXC_DMA_GetIntFlags(regs) == 0) {
			break;
		}
	}
}

static int max32_dma_init(const struct device *dev)
{
	int ret = 0;
	const struct max32_dma_config *cfg = dev->config;

	if (!device_is_ready(cfg->clock)) {
		return -ENODEV;
	}

	/* Enable peripheral clock */
	ret = clock_control_on(cfg->clock, (clock_control_subsys_t) &(cfg->perclk));
	if (ret) {
		return ret;
	}

	ret = Wrap_MXC_DMA_Init(cfg->regs);
	if (ret) {
		return ret;
	}

	/* Acquire all channels so they are available to Zephyr application */
	for (int i = 0; i < cfg->channels; i++) {
		ret = Wrap_MXC_DMA_AcquireChannel(cfg->regs);
		if (ret < 0) {
			break;
		} /* Channels already acquired */
	}

	cfg->irq_configure();

	return 0;
}

static DEVICE_API(dma, max32_dma_driver_api) = {
	.config = max32_dma_config,
	.reload = max32_dma_reload,
	.start = max32_dma_start,
	.stop = max32_dma_stop,
	.get_status = max32_dma_get_status,
};

#define MAX32_DMA_IRQ_CONNECT(n, inst)                                                             \
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(inst, n, irq), DT_INST_IRQ_BY_IDX(inst, n, priority),       \
		    max32_dma_isr, DEVICE_DT_INST_GET(inst), 0);                                   \
	irq_enable(DT_INST_IRQ_BY_IDX(inst, n, irq));

#define CONFIGURE_ALL_IRQS(inst, n) LISTIFY(n, MAX32_DMA_IRQ_CONNECT, (), inst)

#define MAX32_DMA_INIT(inst)                                                                       \
	static struct max32_dma_data dma##inst##_data[DT_INST_PROP(inst, dma_channels)];           \
	static void max32_dma##inst##_irq_configure(void)                                          \
	{                                                                                          \
		CONFIGURE_ALL_IRQS(inst, DT_NUM_IRQS(DT_DRV_INST(inst)));                          \
	}                                                                                          \
	static const struct max32_dma_config dma##inst##_cfg = {                                   \
		.regs = (mxc_dma_regs_t *)DT_INST_REG_ADDR(inst),                                  \
		.clock = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                                 \
		.perclk.bus = DT_INST_CLOCKS_CELL(inst, offset),                                   \
		.perclk.bit = DT_INST_CLOCKS_CELL(inst, bit),                                      \
		.channels = DT_INST_PROP(inst, dma_channels),                                      \
		.irq_configure = max32_dma##inst##_irq_configure,                                  \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, &max32_dma_init, NULL, &dma##inst##_data, &dma##inst##_cfg,    \
			      PRE_KERNEL_1, CONFIG_DMA_INIT_PRIORITY, &max32_dma_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MAX32_DMA_INIT)
