/*
 * Copyright (c) 2025 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <soc.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>

#define DT_DRV_COMPAT nuvoton_npcx_gdma

LOG_MODULE_REGISTER(dma_npcx, CONFIG_DMA_LOG_LEVEL);

enum {
	DMA_NPCX_GDMAMS_SOFT = 0,
	DMA_NPCX_GDMAMS_REQ0,
	DMA_NPCX_GDMAMS_REQ1,
};

enum {
	DMA_NPCX_GDMA_TWS_1B = 0,
	DMA_NPCX_GDMA_TWS_2B,
	DMA_NPCX_GDMA_TWS_4B,
};

enum {
	DMA_NPCX_BURST_LEN_1B  = 1, /* Burst length is 1 byte */
	DMA_NPCX_BURST_LEN_2B  = 2, /* Burst length is 2 bytes */
	DMA_NPCX_BURST_LEN_4B  = 4,  /* Burst length is 4 bytes */
	DMA_NPCX_BURST_LEN_16B = 16  /* Burst length is 16 bytes */
};

struct dma_npcx_ch_data {
	uint32_t channel_dir;
	void *user_data;
	dma_callback_t callback; /* GDMA transfer finish callback function */
};

/* Device config */
struct dma_npcx_config {
	/* DMA controller base address */
	struct gdma_reg **reg_base;
	struct npcx_clk_cfg clk_cfg;
	void (*irq_config)(void);
};

struct dma_npcx_dev_data {
	struct dma_context ctx;
	struct dma_npcx_ch_data *channels;
};

/* Driver convenience defines */
#define HAL_INSTANCE(dev, channel)                                                                 \
	((struct gdma_reg *)(((const struct dma_npcx_config *)(dev)->config)->reg_base[channel]))

static void dma_set_power_down(const struct device *dev, uint8_t channel, bool enable)
{
	struct gdma_reg *const inst = HAL_INSTANCE(dev, channel);

	if (enable) {
		inst->CONTROL |= BIT(NPCX_DMACTL_GPD);
	} else {
		inst->CONTROL &= ~BIT(NPCX_DMACTL_GPD);
	}
}

static void dma_npcx_isr(const struct device *dev)
{
	struct dma_npcx_dev_data *const dev_data = dev->data;
	struct dma_npcx_ch_data *channel_data;
	int ret = DMA_STATUS_COMPLETE;

	for (int channel = 0; channel < dev_data->ctx.dma_channels; channel++) {
		struct gdma_reg *const inst = HAL_INSTANCE(dev, channel);

		channel_data = &dev_data->channels[channel];

		if (!IS_BIT_SET(inst->CONTROL, NPCX_DMACTL_TC)) {
			continue;
		}

#if defined(CONFIG_DMA_NPCX_GDMA_EX)
		/* Check GDMA Transfer Error */
		if (IS_BIT_SET(inst->CONTROL, NPCX_DMACTL_GDMAERR)) {
			LOG_ERR("GDMA transfer error occurred!");
			ret = -EIO;
		}
#endif

		/* Clear GDMA interrupt flag */
		inst->CONTROL &= ~BIT(NPCX_DMACTL_TC);

		if (channel_data->callback) {
			channel_data->callback(dev, channel_data->user_data, channel, ret);
		}
	}
}

static int dma_set_controller(const struct device *dev, uint8_t channel,
			      struct dma_config *dma_ctrl)
{
	struct gdma_reg *const inst = HAL_INSTANCE(dev, channel);

	inst->CONTROL = 0x0;

	/* Set property for source address */
	switch (dma_ctrl->head_block->source_addr_adj) {
	case DMA_ADDR_ADJ_INCREMENT:
		inst->CONTROL &= ~BIT(NPCX_DMACTL_SADIR);
		inst->CONTROL &= ~BIT(NPCX_DMACTL_SAFIX);
		break;
	case DMA_ADDR_ADJ_DECREMENT:
		inst->CONTROL |= BIT(NPCX_DMACTL_SADIR);
		inst->CONTROL &= ~BIT(NPCX_DMACTL_SAFIX);
		break;
	case DMA_ADDR_ADJ_NO_CHANGE:
		inst->CONTROL |= BIT(NPCX_DMACTL_SAFIX);
		break;
	default:
		return -EINVAL;
	}

	/* Set property for destination address */
	switch (dma_ctrl->head_block->dest_addr_adj) {
	case DMA_ADDR_ADJ_INCREMENT:
		inst->CONTROL &= ~BIT(NPCX_DMACTL_DADIR);
		inst->CONTROL &= ~BIT(NPCX_DMACTL_DAFIX);
		break;
	case DMA_ADDR_ADJ_DECREMENT:
		inst->CONTROL |= BIT(NPCX_DMACTL_DADIR);
		inst->CONTROL &= ~BIT(NPCX_DMACTL_DAFIX);
		break;
	case DMA_ADDR_ADJ_NO_CHANGE:
		inst->CONTROL |= BIT(NPCX_DMACTL_DAFIX);
		break;
	default:
		return -EINVAL;
	}

	switch (dma_ctrl->channel_direction) {
	/* Enable mem to mem data transfer only*/
	case MEMORY_TO_MEMORY:
		SET_FIELD(inst->CONTROL, NPCX_DMACTL_GDMAMS, DMA_NPCX_GDMAMS_SOFT);
		inst->CONTROL &= ~BIT(NPCX_DMACTL_DM);
		break;
	case MEMORY_TO_PERIPHERAL:
		__fallthrough;
	case PERIPHERAL_TO_MEMORY:
		__fallthrough;
	case PERIPHERAL_TO_PERIPHERAL:
		/* demand mode */
		SET_FIELD(inst->CONTROL, NPCX_DMACTL_GDMAMS,
			  channel ? DMA_NPCX_GDMAMS_REQ1 : DMA_NPCX_GDMAMS_REQ0);
		inst->CONTROL |= BIT(NPCX_DMACTL_DM);
		break;
	default:
		return -EINVAL;
	}

	/* Set Transfer Width Select */
	switch (dma_ctrl->source_burst_length) {
	case DMA_NPCX_BURST_LEN_1B:
		SET_FIELD(inst->CONTROL, NPCX_DMACTL_TWS, DMA_NPCX_GDMA_TWS_1B);
		inst->CONTROL &= ~BIT(NPCX_DMACTL_BME);
		break;
	case DMA_NPCX_BURST_LEN_2B:
		SET_FIELD(inst->CONTROL, NPCX_DMACTL_TWS, DMA_NPCX_GDMA_TWS_2B);
		inst->CONTROL &= ~BIT(NPCX_DMACTL_BME);
		break;
	case DMA_NPCX_BURST_LEN_4B:
		SET_FIELD(inst->CONTROL, NPCX_DMACTL_TWS, DMA_NPCX_GDMA_TWS_4B);
		inst->CONTROL &= ~BIT(NPCX_DMACTL_BME);
		break;
	case DMA_NPCX_BURST_LEN_16B:
		SET_FIELD(inst->CONTROL, NPCX_DMACTL_TWS, DMA_NPCX_GDMA_TWS_4B);
		inst->CONTROL |= BIT(NPCX_DMACTL_BME);
		break;
	default:
		return -ENOTSUP;
	}

	inst->SRCB = dma_ctrl->head_block->source_address;
	inst->DSTB = dma_ctrl->head_block->dest_address;
	inst->TCNT = dma_ctrl->head_block->block_size / dma_ctrl->source_burst_length;

	/* Enable/disable stop interrupt of GDMA operation */
	if (dma_ctrl->dma_callback) {
		inst->CONTROL |= BIT(NPCX_DMACTL_SIEN);
	} else {
		inst->CONTROL &= ~BIT(NPCX_DMACTL_SIEN);
	}

	return 0;
}

static int dma_trans_start(const struct device *dev, uint8_t channel)
{
	struct gdma_reg *const inst = HAL_INSTANCE(dev, channel);

	/* Check there is no GDMA transaction */
	if (IS_BIT_SET(inst->CONTROL, NPCX_DMACTL_GDMAEN) ||
	    IS_BIT_SET(inst->CONTROL, NPCX_DMACTL_TC)) {
		return -EBUSY;
	}

	/* Enable GDMA operation */
	inst->CONTROL |= BIT(NPCX_DMACTL_GDMAEN);

	/* Software triggered GDMA request */
	if (GET_FIELD(inst->CONTROL, NPCX_DMACTL_GDMAMS) == DMA_NPCX_GDMAMS_SOFT) {
		inst->CONTROL |= BIT(NPCX_DMACTL_SOFTREQ);
	}

	return 0;
}

static int dma_trans_stop(const struct device *dev, uint8_t channel)
{
	struct gdma_reg *const inst = HAL_INSTANCE(dev, channel);

	/* Disable GDMA operation */
	inst->CONTROL &= ~BIT(NPCX_DMACTL_GDMAEN);

	return 0;
}

static int dma_npcx_configure(const struct device *dev, uint32_t channel, struct dma_config *cfg)
{
	struct dma_npcx_dev_data *const dev_data = dev->data;
	struct dma_npcx_ch_data *channel_data;
	struct gdma_reg *const inst = HAL_INSTANCE(dev, channel);

	uint32_t block_size = cfg->head_block->block_size;
	uintptr_t src_addr = cfg->head_block->source_address;
	uintptr_t dst_addr = cfg->head_block->dest_address;

	/* Check channel is valid */
	if (channel >= dev_data->ctx.dma_channels) {
		LOG_ERR("out of range DMA channel %d", channel);
		return -EINVAL;
	}

	/* Check there is no GDMA transaction */
	if (IS_BIT_SET(inst->CONTROL, NPCX_DMACTL_GDMAEN) ||
	    IS_BIT_SET(inst->CONTROL, NPCX_DMACTL_TC)) {
		return -EBUSY;
	}

	channel_data = &dev_data->channels[channel];

	if (cfg->source_burst_length != cfg->dest_burst_length) {
		LOG_ERR("Burst length mismatch between source and destination");
		return -EINVAL;
	}

	/* Check the source address is aligned */
	if ((src_addr % cfg->source_burst_length) != 0) {
		LOG_ERR("Source Address Not Aligned (0x%lx)", src_addr);
		return -EINVAL;
	}

	/* Check the destination address is aligned */
	if ((dst_addr % cfg->dest_burst_length) != 0) {
		LOG_ERR("Destination Address Not Aligned (0x%lx)", dst_addr);
		return -EINVAL;
	}

	/* Check the size is aligned */
	if ((block_size % (cfg->source_burst_length)) != 0) {
		LOG_ERR("Size Not Aligned");
		return -EINVAL;
	}

	/* Check source and destination region not overlay */
	if (((src_addr + block_size) > dst_addr) && ((dst_addr + block_size) > src_addr)) {
		LOG_ERR("Transaction Region Overlap");
		return -EFAULT;
	}

	channel_data->channel_dir = cfg->channel_direction;
	channel_data->callback = cfg->dma_callback;
	channel_data->user_data = cfg->user_data;

	return dma_set_controller(dev, channel, cfg);
}

static int dma_npcx_start(const struct device *dev, uint32_t channel)
{
	struct dma_npcx_dev_data *const dev_data = dev->data;

	if (channel >= dev_data->ctx.dma_channels) {
		LOG_ERR("out of range DMA channel %d", channel);
		return -EINVAL;
	}

	return dma_trans_start(dev, channel);
}

static int dma_npcx_stop(const struct device *dev, uint32_t channel)
{
	struct dma_npcx_dev_data *const dev_data = dev->data;

	if (channel >= dev_data->ctx.dma_channels) {
		LOG_ERR("out of range DMA channel %d", channel);
		return -EINVAL;
	}

	dma_trans_stop(dev, channel);

	return 0;
}

static int dma_npcx_get_status(const struct device *dev, uint32_t channel,
			       struct dma_status *status)
{
	struct dma_npcx_dev_data *const dev_data = dev->data;
	struct dma_npcx_ch_data *channel_data;
	uint32_t cnt, bus_width;

	if (channel >= dev_data->ctx.dma_channels) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	struct gdma_reg *const inst = HAL_INSTANCE(dev, channel);

	channel_data = &dev_data->channels[channel];
	bus_width = IS_BIT_SET(inst->CONTROL, NPCX_DMACTL_BME)
			     ? DMA_NPCX_BURST_LEN_16B
			     : (1 << GET_FIELD(inst->CONTROL, NPCX_DMACTL_TWS));

	status->dir = channel_data->channel_dir;
	status->busy = IS_BIT_SET(inst->CONTROL, NPCX_DMACTL_GDMAEN);
	if (status->busy == true) {
		cnt = inst->CTCNT;
		status->pending_length = bus_width * cnt;
		status->total_copied = (inst->TCNT - cnt) * bus_width;
	} else {
		status->pending_length = 0;
		status->total_copied = inst->TCNT * bus_width;
	}

	return 0;
}

static bool dma_npcx_chan_filter(const struct device *dev, int ch, void *filter_param)
{
	struct dma_npcx_dev_data *const dev_data = dev->data;

	if (ch >= dev_data->ctx.dma_channels) {
		LOG_ERR("Invalid DMA channel index %d", ch);
		return false;
	}

	if ((filter_param != NULL) && (*((uint32_t *)filter_param) != ch)) {
		return false;
	}

	return true;
}

static int dma_npcx_init(const struct device *dev)
{
	const struct dma_npcx_config *const dev_cfg = dev->config;
	struct dma_npcx_dev_data *const dev_data = dev->data;
	const struct device *const clk_dev = DEVICE_DT_GET(NPCX_CLK_CTRL_NODE);
	int ret;

	if (!device_is_ready(clk_dev)) {
		LOG_ERR("%s device not ready", clk_dev->name);
		return -ENODEV;
	}

	ret = clock_control_on(clk_dev, (clock_control_subsys_t)&dev_cfg->clk_cfg);
	if (ret < 0) {
		LOG_ERR("Turn on GDMA clock fail %d", ret);
		return ret;
	}

	/* default as below  */
	for (uint8_t ch = 0; ch < dev_data->ctx.dma_channels; ch++) {
		/* power up both channel when init */
		dma_set_power_down(dev, ch, 0);
	}

	/* Configure DMA interrupt and enable it */
	dev_cfg->irq_config();

	return 0;
}

static const struct dma_driver_api npcx_driver_api = {
		.config = dma_npcx_configure,
		.start = dma_npcx_start,
		.stop = dma_npcx_stop,
		.get_status = dma_npcx_get_status,
		.chan_filter = dma_npcx_chan_filter,
};

#define DMA_NPCX_GDMA_CH_REG_OFFSET(n) DT_INST_PROP(n, chan_offset)
#define DMA_NPCX_GDMA_CH_REG(channel, n)                                                           \
	(struct gdma_reg *)(DT_INST_REG_ADDR(n) + (channel) * (DMA_NPCX_GDMA_CH_REG_OFFSET(n)))
#define DMA_NPCX_INIT(n)                                                                           \
	static struct gdma_reg *gdma_reg_ch_base_##n[] = {                                         \
		LISTIFY(DT_INST_PROP(n, dma_channels), DMA_NPCX_GDMA_CH_REG, (,), n)};             \
	static void dma_npcx_##n##_irq_config(void)                                                \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), dma_npcx_isr,               \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
	static struct dma_npcx_ch_data dma_npcx_##n##_channels[DT_INST_PROP(n, dma_channels)];     \
	ATOMIC_DEFINE(dma_npcx_atomic_##n, DT_INST_PROP(n, dma_channels));                         \
	static struct dma_npcx_dev_data dma_npcx_data_##n = {                                      \
		.ctx.magic = DMA_MAGIC,                                                            \
		.ctx.dma_channels = DT_INST_PROP(n, dma_channels),                                 \
		.ctx.atomic = dma_npcx_atomic_##n,                                                 \
		.channels = dma_npcx_##n##_channels,                                               \
	};                                                                                         \
	static const struct dma_npcx_config npcx_config_##n = {                                    \
		.reg_base = &gdma_reg_ch_base_##n[0],                                              \
		.clk_cfg = NPCX_DT_CLK_CFG_ITEM(n),                                                \
		.irq_config = dma_npcx_##n##_irq_config,                                           \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, dma_npcx_init, NULL, &dma_npcx_data_##n, &npcx_config_##n,        \
			      PRE_KERNEL_1, CONFIG_DMA_INIT_PRIORITY, &npcx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DMA_NPCX_INIT)
