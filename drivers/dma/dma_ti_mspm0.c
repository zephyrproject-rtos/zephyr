/*
 * Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_mspm0_dma

#include <stdio.h>
#include <string.h>
#include <soc.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <driverlib/dl_dma.h>

LOG_MODULE_REGISTER(ti_mspm0_dma, CONFIG_DMA_LOG_LEVEL);

#define DMA_TI_MSPM0_BASE_CHANNEL_NUM	1

/* Data Transfer Width */
#define DMA_TI_MSPM0_DATAWIDTH_BYTE	1
#define DMA_TI_MSPM0_DATAWIDTH_HALF	2
#define DMA_TI_MSPM0_DATAWIDTH_WORD	3
#define DMA_TI_MSPM0_DATAWIDTH_LONG	4

#define DMA_TI_MSPM0_MAX_CHANNEL	DT_INST_PROP(0, dma_channels)
BUILD_ASSERT((DMA_TI_MSPM0_MAX_CHANNEL != 0), "dma-channels property required");

struct dma_ti_mspm0_config {
	DMA_Regs *regs;
	void (*irq_config_func)(void);
};

struct dma_ti_mspm0_channel_data {
	dma_callback_t dma_callback;
	void *user_data;
	uint8_t direction;
	bool busy;
};

struct dma_ti_mspm0_data {
	struct dma_context dma_ctx;
	struct k_spinlock lock;
	struct dma_ti_mspm0_channel_data ch_data[DMA_TI_MSPM0_MAX_CHANNEL];
};

static inline int dma_ti_mspm0_get_memory_increment(uint8_t adj,
						    uint32_t *increment)
{
	if (increment == NULL) {
		return -EINVAL;
	}

	switch (adj) {
	case DMA_ADDR_ADJ_INCREMENT:
		*increment = DL_DMA_ADDR_INCREMENT;
		break;
	case DMA_ADDR_ADJ_NO_CHANGE:
		*increment = DL_DMA_ADDR_UNCHANGED;
		break;
	case DMA_ADDR_ADJ_DECREMENT:
		*increment = DL_DMA_ADDR_DECREMENT;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static inline int dma_ti_mspm0_get_dstdatawidth(uint8_t wd, uint32_t *dwidth)
{
	if (dwidth == NULL) {
		return -EINVAL;
	}

	switch (wd) {
	case DMA_TI_MSPM0_DATAWIDTH_BYTE:
		*dwidth = DL_DMA_WIDTH_BYTE;
		break;
	case DMA_TI_MSPM0_DATAWIDTH_HALF:
		*dwidth = DMA_DMACTL_DMADSTWDTH_HALF;
		break;
	case DMA_TI_MSPM0_DATAWIDTH_WORD:
		*dwidth = DMA_DMACTL_DMADSTWDTH_WORD;
		break;
	case DMA_TI_MSPM0_DATAWIDTH_LONG:
		*dwidth = DMA_DMACTL_DMADSTWDTH_LONG;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static inline int dma_ti_mspm0_get_srcdatawidth(uint8_t wd, uint32_t *dwidth)
{
	if (dwidth == NULL) {
		return -EINVAL;
	}

	switch (wd) {
	case DMA_TI_MSPM0_DATAWIDTH_BYTE:
		*dwidth = DL_DMA_WIDTH_BYTE;
		break;
	case DMA_TI_MSPM0_DATAWIDTH_HALF:
		*dwidth = DMA_DMACTL_DMASRCWDTH_HALF;
		break;
	case DMA_TI_MSPM0_DATAWIDTH_WORD:
		*dwidth = DMA_DMACTL_DMASRCWDTH_WORD;
		break;
	case DMA_TI_MSPM0_DATAWIDTH_LONG:
		*dwidth = DMA_DMACTL_DMASRCWDTH_LONG;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int dma_ti_mspm0_configure(const struct device *dev, uint32_t channel,
				  struct dma_config *config)
{
	uint32_t temp;
	const struct dma_ti_mspm0_config *cfg = dev->config;
	struct dma_ti_mspm0_data *dma_data = dev->data;
	struct dma_ti_mspm0_channel_data *data = NULL;
	struct dma_block_config *b_cfg = config->head_block;
	DL_DMA_Config dma_cfg = {0};

	if ((config == NULL) || (channel > DMA_TI_MSPM0_MAX_CHANNEL)) {
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

	dma_cfg.srcIncrement = temp;

	if (dma_ti_mspm0_get_memory_increment(b_cfg->dest_addr_adj, &temp)) {
		LOG_ERR("Invalid Destination address increment");
		return -EINVAL;
	}

	dma_cfg.destIncrement = temp;

	if (dma_ti_mspm0_get_dstdatawidth(config->dest_data_size, &temp)) {
		LOG_ERR("Invalid Destination data width");
		return -EINVAL;
	}

	dma_cfg.destWidth = temp;

	if (dma_ti_mspm0_get_srcdatawidth(config->source_data_size, &temp)) {
		LOG_ERR("Invalid Source data width");
		return -EINVAL;
	}

	dma_cfg.srcWidth = temp;
	data->direction = config->channel_direction;
	data->dma_callback = config->dma_callback;
	data->user_data = config->user_data;
	dma_cfg.transferMode = DL_DMA_SINGLE_TRANSFER_MODE,
	dma_cfg.extendedMode = DL_DMA_NORMAL_MODE,
	dma_cfg.triggerType = DL_DMA_TRIGGER_TYPE_EXTERNAL;
	dma_cfg.trigger	= config->dma_slot;

	K_SPINLOCK(&dma_data->lock) {
		DL_DMA_clearInterruptStatus(cfg->regs,
					    (channel + DMA_TI_MSPM0_BASE_CHANNEL_NUM));
		DL_DMA_setTransferSize(cfg->regs, channel, b_cfg->block_size);
		DL_DMA_initChannel(cfg->regs, channel, &dma_cfg);
		DL_DMA_setSrcAddr(cfg->regs, channel, b_cfg->source_address);
		DL_DMA_setDestAddr(cfg->regs, channel, b_cfg->dest_address);
		DL_DMA_enableInterrupt(cfg->regs,
				       (channel + DMA_TI_MSPM0_BASE_CHANNEL_NUM));
		data->busy = true;
	}

	LOG_DBG("DMA Channel %u configured", channel);

	return 0;
}

static int dma_ti_mspm0_start(const struct device *dev, const uint32_t channel)
{
	const struct dma_ti_mspm0_config *cfg = dev->config;

	if (channel > DMA_TI_MSPM0_MAX_CHANNEL) {
		return -EINVAL;
	}

	DL_DMA_enableChannel(cfg->regs, channel);

	return 0;
}

static int dma_ti_mspm0_stop(const struct device *dev, const uint32_t channel)
{
	const struct dma_ti_mspm0_config *cfg = dev->config;
	struct dma_ti_mspm0_data *data = dev->data;

	if (channel > DMA_TI_MSPM0_MAX_CHANNEL) {
		return -EINVAL;
	}

	DL_DMA_disableChannel(cfg->regs, channel);
	data->ch_data[channel].busy = false;

	return 0;
}

static int dma_ti_mspm0_reload(const struct device *dev, uint32_t channel,
			       uint32_t src_addr, uint32_t dest_addr, size_t size)
{
	const struct dma_ti_mspm0_config *cfg = dev->config;
	struct dma_ti_mspm0_channel_data *data = NULL;
	struct dma_ti_mspm0_data *dma_data = dev->data;

	if (channel > DMA_TI_MSPM0_MAX_CHANNEL) {
		return -EINVAL;
	}

	data = &dma_data->ch_data[channel];
	switch (data->direction) {
	case PERIPHERAL_TO_MEMORY:
		DL_DMA_setDestAddr(cfg->regs, channel, dest_addr);
		break;
	case MEMORY_TO_PERIPHERAL:
		DL_DMA_setSrcAddr(cfg->regs, channel, src_addr);
		break;
	default:
		LOG_ERR("Unsupported data direction");
		return -ENOTSUP;
	}

	DL_DMA_setTransferSize(cfg->regs, channel, size);
	data->busy = true;

	return 0;
}

static int dma_ti_mspm0_get_status(const struct device *dev, uint32_t channel,
				   struct dma_status *stat)
{
	const struct dma_ti_mspm0_config *cfg = dev->config;
	struct dma_ti_mspm0_data *dma_data = dev->data;
	struct dma_ti_mspm0_channel_data *data;

	if (channel > DMA_TI_MSPM0_MAX_CHANNEL) {
		return -EINVAL;
	}

	data = &dma_data->ch_data[channel];
	stat->pending_length = DL_DMA_getTransferSize(cfg->regs, channel);
	stat->dir = data->direction;
	stat->busy = data->busy;

	return 0;
}

static inline void dma_ti_mspm0_isr(const struct device *dev)
{
	int channel;
	const struct dma_ti_mspm0_config *cfg = dev->config;
	struct dma_ti_mspm0_data *dma_data = dev->data;
	struct dma_ti_mspm0_channel_data *data;

	channel = DL_DMA_getPendingInterrupt(cfg->regs);
	channel -= DMA_TI_MSPM0_BASE_CHANNEL_NUM;

	if ((channel < 0) || (channel > DMA_TI_MSPM0_MAX_CHANNEL)) {
		return;
	}

	data = &dma_data->ch_data[channel];
	DL_DMA_disableChannel(cfg->regs, channel);

	if (data->dma_callback !=  NULL) {
		data->busy = false;
		data->dma_callback(dev, data->user_data, channel, 0);
	}
}

static int dma_ti_mspm0_init(const struct device *dev)
{
	const struct dma_ti_mspm0_config *cfg = dev->config;

	if (cfg->irq_config_func != NULL) {
		cfg->irq_config_func();
	}

	return 0;
};

static DEVICE_API(dma, dma_ti_mspm0_api) = {
	.config		= dma_ti_mspm0_configure,
	.start		= dma_ti_mspm0_start,
	.stop		= dma_ti_mspm0_stop,
	.reload		= dma_ti_mspm0_reload,
	.get_status	= dma_ti_mspm0_get_status,
};

#define MSPM0_DMA_INIT(inst)							\
	static inline void dma_ti_mspm0_irq_cfg_##inst(void)			\
	{									\
		irq_disable(DT_INST_IRQN(inst));				\
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority),	\
			    dma_ti_mspm0_isr, DEVICE_DT_INST_GET(inst), 0);	\
										\
		irq_enable(DT_INST_IRQN(inst));					\
	}									\
										\
	static const struct dma_ti_mspm0_config dma_cfg_##inst = {		\
		.regs		 = (DMA_Regs *)DT_INST_REG_ADDR(inst),		\
		.irq_config_func = dma_ti_mspm0_irq_cfg_##inst,			\
	};									\
	struct dma_ti_mspm0_data dma_data_##inst;				\
										\
	DEVICE_DT_INST_DEFINE(inst, &dma_ti_mspm0_init, NULL,			\
			      &dma_data_##inst, &dma_cfg_##inst,		\
			      PRE_KERNEL_1, CONFIG_DMA_INIT_PRIORITY,		\
			      &dma_ti_mspm0_api);

DT_INST_FOREACH_STATUS_OKAY(MSPM0_DMA_INIT);
