/*
 * SPDX-FileCopyrightText: 2026 Linumiz
 * SPDX-FileCopyrightText: 2026 Texas Instruments Incorporated

 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_mspm0_dma

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(ti_mspm0_dma, CONFIG_DMA_LOG_LEVEL);

#define DMA_TI_MSPM0_BASE_CHANNEL_NUM	1
#define DMA_GET_CHANNEL_FROM_STATUS(status)	\
			((status) - DMA_TI_MSPM0_BASE_CHANNEL_NUM)

/* DMA register offsets */
#define DMA_MSPM0_CPU_INT_IIDX   0x1020
#define DMA_MSPM0_CPU_INT_IMASK  0x1028
#define DMA_MSPM0_DMATCTL(n)     (0x1110 + ((n) * 0x4))
#define DMA_MSPM0_DMACTL(n)      (0x1200 + ((n) * 0x10))
#define DMA_MSPM0_DMASA(n)       (0x1204 + ((n) * 0x10))
#define DMA_MSPM0_DMADA(n)       (0x1208 + ((n) * 0x10))
#define DMA_MSPM0_DMASZ(n)       (0x120C + ((n) * 0x10))

/* dmactl bits (per-channel) */
#define DMA_MSPM0_CTL_DMAREQ    BIT(0)
#define DMA_MSPM0_CTL_DMAEN     BIT(1)
#define DMA_MSPM0_CTL_SRCWDTH   GENMASK(10, 8)
#define DMA_MSPM0_CTL_DSTWDTH   GENMASK(14, 12)
#define DMA_MSPM0_CTL_SRCINCR   GENMASK(19, 16)
#define DMA_MSPM0_CTL_DSTINCR   GENMASK(23, 20)
#define DMA_MSPM0_CTL_EM        GENMASK(25, 24)
#define DMA_MSPM0_CTL_TM        GENMASK(29, 28)

#define DMA_MSPM0_WIDTH_BYTE     0x0U
#define DMA_MSPM0_WIDTH_HALF     0x1U
#define DMA_MSPM0_WIDTH_WORD     0x2U
#define DMA_MSPM0_WIDTH_LONG     0x3U

#define DMA_MSPM0_INCR_UNCHANGED 0x0U
#define DMA_MSPM0_INCR_DECREMENT 0x2U
#define DMA_MSPM0_INCR_INCREMENT 0x3U

#define DMA_MSPM0_EM_NORMAL      0x0U
#define DMA_MSPM0_TM_SINGLE      0x0U

/* dmatctl bits (per-channel, dmatctl[n]) */
#define DMA_MSPM0_TCTL_DMATSEL GENMASK(5, 0)

/* cpu_int / gen_event IIDX status codes: channel N reports (N + 1) */
#define DMA_MSPM0_IIDX_NONE 0x00U

/* Data Transfer Width, in bytes -- matches Zephyr's source/dest_data_size */
#define DMA_TI_MSPM0_DATAWIDTH_BYTE	1
#define DMA_TI_MSPM0_DATAWIDTH_HALF	2
#define DMA_TI_MSPM0_DATAWIDTH_WORD	4
#define DMA_TI_MSPM0_DATAWIDTH_LONG	8

struct dma_ti_mspm0_config {
	mm_reg_t base;
	uint8_t dma_max_channels;
	void (*irq_config_func)(void);
	uint8_t num_full_channels;
};

struct dma_ti_mspm0_channel_data {
	dma_callback_t dma_callback;
	void *user_data;
	uint8_t direction;
	bool busy;
	uint8_t data_size;
};

struct dma_ti_mspm0_data {
	struct dma_context dma_ctx;
	struct k_spinlock lock;
	struct dma_ti_mspm0_channel_data *ch_data;
};

static inline int dma_ti_mspm0_get_memory_increment(uint8_t adj,
						    uint32_t *increment)
{
	if (increment == NULL) {
		return -EINVAL;
	}

	switch (adj) {
	case DMA_ADDR_ADJ_INCREMENT:
		*increment = DMA_MSPM0_INCR_INCREMENT;
		break;
	case DMA_ADDR_ADJ_NO_CHANGE:
		*increment = DMA_MSPM0_INCR_UNCHANGED;
		break;
	case DMA_ADDR_ADJ_DECREMENT:
		*increment = DMA_MSPM0_INCR_DECREMENT;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static inline int dma_ti_mspm0_get_datawidth(uint8_t wd, uint32_t *width)
{
	if (width == NULL) {
		return -EINVAL;
	}

	switch (wd) {
	case DMA_TI_MSPM0_DATAWIDTH_BYTE:
		*width = DMA_MSPM0_WIDTH_BYTE;
		break;
	case DMA_TI_MSPM0_DATAWIDTH_HALF:
		*width = DMA_MSPM0_WIDTH_HALF;
		break;
	case DMA_TI_MSPM0_DATAWIDTH_WORD:
		*width = DMA_MSPM0_WIDTH_WORD;
		break;
	case DMA_TI_MSPM0_DATAWIDTH_LONG:
		*width = DMA_MSPM0_WIDTH_LONG;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static inline bool dma_ti_mspm0_is_full_channel(const struct dma_ti_mspm0_config *cfg,
						uint32_t channel)
{
	return channel < cfg->num_full_channels;
}

static int dma_ti_mspm0_configure(const struct device *dev, uint32_t channel,
				  struct dma_config *config)
{
	uint32_t ctl = 0;
	uint32_t temp;
	const struct dma_ti_mspm0_config *cfg = dev->config;
	struct dma_ti_mspm0_data *dma_data = dev->data;
	struct dma_ti_mspm0_channel_data *data = NULL;
	struct dma_block_config *b_cfg = NULL;

	if ((config == NULL) || (channel >= cfg->dma_max_channels)) {
		return -EINVAL;
	}

	b_cfg = config->head_block;
	if (b_cfg == NULL) {
		return -EINVAL;
	}

	data = &dma_data->ch_data[channel];

	if (data->busy != false) {
		return -EBUSY;
	}

	if (config->dest_data_size != config->source_data_size) {
		LOG_ERR("Source and Destination data width is not same");
		return -EINVAL;
	}

	if (dma_ti_mspm0_get_memory_increment(b_cfg->source_addr_adj, &temp)) {
		LOG_ERR("Invalid Source address increment");
		return -EINVAL;
	}

	ctl |= FIELD_PREP(DMA_MSPM0_CTL_SRCINCR, temp);

	if (dma_ti_mspm0_get_memory_increment(b_cfg->dest_addr_adj, &temp)) {
		LOG_ERR("Invalid Destination address increment");
		return -EINVAL;
	}

	ctl |= FIELD_PREP(DMA_MSPM0_CTL_DSTINCR, temp);

	if (dma_ti_mspm0_get_datawidth(config->source_data_size, &temp)) {
		LOG_ERR("Invalid Source data width");
		return -EINVAL;
	}

	ctl |= FIELD_PREP(DMA_MSPM0_CTL_SRCWDTH, temp);

	if (dma_ti_mspm0_get_datawidth(config->dest_data_size, &temp)) {
		LOG_ERR("Invalid Destination data width");
		return -EINVAL;
	}

	ctl |= FIELD_PREP(DMA_MSPM0_CTL_DSTWDTH, temp);

	if (config->channel_direction == PERIPHERAL_TO_MEMORY) {
		data->data_size = config->source_data_size;
	} else {
		data->data_size = config->dest_data_size;
	}

	ctl |= FIELD_PREP(DMA_MSPM0_CTL_TM, DMA_MSPM0_TM_SINGLE);
	ctl |= FIELD_PREP(DMA_MSPM0_CTL_EM, DMA_MSPM0_EM_NORMAL);

	data->direction = config->channel_direction;
	data->dma_callback = config->dma_callback;
	data->user_data = config->user_data;

	K_SPINLOCK(&dma_data->lock) {
		sys_clear_bit(cfg->base + DMA_MSPM0_CPU_INT_IMASK, channel);
		sys_write32(b_cfg->block_size / data->data_size,
			    cfg->base + DMA_MSPM0_DMASZ(channel));
		sys_write32(FIELD_PREP(DMA_MSPM0_TCTL_DMATSEL, config->dma_slot),
			    cfg->base + DMA_MSPM0_DMATCTL(channel));
		sys_write32(ctl, cfg->base + DMA_MSPM0_DMACTL(channel));
		sys_write32(b_cfg->source_address, cfg->base + DMA_MSPM0_DMASA(channel));
		sys_write32(b_cfg->dest_address, cfg->base + DMA_MSPM0_DMADA(channel));
		sys_set_bit(cfg->base + DMA_MSPM0_CPU_INT_IMASK, channel);
		data->busy = true;
	}

	LOG_DBG("DMA Channel %u configured", channel);

	return 0;
}

static int dma_ti_mspm0_start(const struct device *dev, const uint32_t channel)
{
	const struct dma_ti_mspm0_config *cfg = dev->config;

	if (channel >= cfg->dma_max_channels) {
		return -EINVAL;
	}

	sys_set_bits(cfg->base + DMA_MSPM0_DMACTL(channel), DMA_MSPM0_CTL_DMAEN);

	return 0;
}

static int dma_ti_mspm0_stop(const struct device *dev, const uint32_t channel)
{
	const struct dma_ti_mspm0_config *cfg = dev->config;
	struct dma_ti_mspm0_data *data = dev->data;

	if (channel >= cfg->dma_max_channels) {
		return -EINVAL;
	}

	sys_clear_bits(cfg->base + DMA_MSPM0_DMACTL(channel), DMA_MSPM0_CTL_DMAEN);
	data->ch_data[channel].busy = false;

	return 0;
}

static int dma_ti_mspm0_reload(const struct device *dev, uint32_t channel,
			       uint32_t src_addr, uint32_t dest_addr, size_t size)
{
	const struct dma_ti_mspm0_config *cfg = dev->config;
	struct dma_ti_mspm0_channel_data *data = NULL;
	struct dma_ti_mspm0_data *dma_data = dev->data;

	if (channel >= cfg->dma_max_channels) {
		return -EINVAL;
	}

	data = &dma_data->ch_data[channel];
	sys_write32(src_addr, cfg->base + DMA_MSPM0_DMASA(channel));
	sys_write32(dest_addr, cfg->base + DMA_MSPM0_DMADA(channel));
	sys_write32(size / data->data_size, cfg->base + DMA_MSPM0_DMASZ(channel));
	data->busy = true;

	return 0;
}

static int dma_ti_mspm0_get_status(const struct device *dev, uint32_t channel,
				   struct dma_status *stat)
{
	const struct dma_ti_mspm0_config *cfg = dev->config;
	struct dma_ti_mspm0_data *dma_data = dev->data;
	struct dma_ti_mspm0_channel_data *data;

	if (channel >= cfg->dma_max_channels) {
		return -EINVAL;
	}

	data = &dma_data->ch_data[channel];
	stat->pending_length = sys_read32(cfg->base + DMA_MSPM0_DMASZ(channel));
	stat->dir = data->direction;
	stat->busy = data->busy;

	return 0;
}

static inline void dma_ti_mspm0_isr(const struct device *dev)
{
	int status;
	uint32_t channel;
	const struct dma_ti_mspm0_config *cfg = dev->config;
	struct dma_ti_mspm0_data *dma_data = dev->data;
	struct dma_ti_mspm0_channel_data *data;

	/* Reading IIDX also latches-clears the highest priority pending flag */
	status = sys_read32(cfg->base + DMA_MSPM0_CPU_INT_IIDX);
	if (status == DMA_MSPM0_IIDX_NONE) {
		return;
	}

	channel = DMA_GET_CHANNEL_FROM_STATUS(status);
	if (channel >= cfg->dma_max_channels) {
		return;
	}

	data = &dma_data->ch_data[channel];
	sys_clear_bits(cfg->base + DMA_MSPM0_DMACTL(channel), DMA_MSPM0_CTL_DMAEN);

	data->busy = false;
	if (data->dma_callback != NULL) {
		data->dma_callback(dev, data->user_data, channel, DMA_STATUS_COMPLETE);
	}
}

static int dma_ti_mspm0_init(const struct device *dev)
{
	const struct dma_ti_mspm0_config *cfg = dev->config;

	if (cfg->irq_config_func != NULL) {
		cfg->irq_config_func();
	}

	return 0;
}

static DEVICE_API(dma, dma_ti_mspm0_api) = {
	.config		= dma_ti_mspm0_configure,
	.start		= dma_ti_mspm0_start,
	.stop		= dma_ti_mspm0_stop,
	.reload		= dma_ti_mspm0_reload,
	.get_status	= dma_ti_mspm0_get_status,
};

#define MSPM0_DMA_INIT(inst)							\
										\
	BUILD_ASSERT(DT_INST_NODE_HAS_PROP(inst, dma_channels),			\
		     "DMA channels is required");				\
										\
	BUILD_ASSERT(DT_INST_PROP(inst, ti_num_full_channels) <=			\
		     DT_INST_PROP(inst, dma_channels),				\
		     "ti,num-full-channels can't exceed dma-channels");	\
										\
	static inline void dma_ti_mspm0_irq_cfg_##inst(void)			\
	{									\
		irq_disable(DT_INST_IRQN(inst));				\
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority),	\
			    dma_ti_mspm0_isr, DEVICE_DT_INST_GET(inst), 0);	\
										\
		irq_enable(DT_INST_IRQN(inst));					\
	}									\
										\
	static struct dma_ti_mspm0_channel_data					\
			channel_data_##inst[DT_INST_PROP(inst, dma_channels)];	\
										\
	static const struct dma_ti_mspm0_config dma_cfg_##inst = {		\
		.num_full_channels = DT_INST_PROP(inst, ti_num_full_channels),	\
		.base		  = DT_INST_REG_ADDR(inst),	\
		.dma_max_channels = DT_INST_PROP(inst, dma_channels),		\
		.irq_config_func  = dma_ti_mspm0_irq_cfg_##inst,		\
	};									\
										\
	static struct dma_ti_mspm0_data dma_data_##inst = {			\
		.ch_data = channel_data_##inst,					\
	};									\
										\
	DEVICE_DT_INST_DEFINE(inst, &dma_ti_mspm0_init, NULL,			\
			      &dma_data_##inst, &dma_cfg_##inst,		\
			      PRE_KERNEL_1, CONFIG_DMA_INIT_PRIORITY,		\
			      &dma_ti_mspm0_api);

DT_INST_FOREACH_STATUS_OKAY(MSPM0_DMA_INIT);
