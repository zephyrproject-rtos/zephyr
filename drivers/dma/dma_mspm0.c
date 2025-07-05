/*
 * Copyright (c) 2025 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_mspm0_dma

#include <errno.h>

#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/dma.h>
#include <soc.h>

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(ti_dma, CONFIG_DMA_LOG_LEVEL);

#define DMA_MAX_CHANNEL	DT_INST_PROP(0, dma_channels)

struct dma_mspm0_config {
	DMA_Regs *base;
};

struct dma_mspm0_channel_data {
	uint8_t direction;
	bool busy;
	dma_callback_t dma_callback;
	void *user_data;
};

struct dma_mspm0_data {
	struct dma_context dma_ctx;
	struct dma_mspm0_channel_data ch_data[DMA_MAX_CHANNEL];
};

static inline int mspm0_get_memory_increment(uint8_t adj, uint32_t *increment)
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

static inline int mspm0_get_datawidth(uint8_t wd, uint32_t *dwidth)
{
	if (dwidth == NULL) {
		return -EINVAL;
	}

	switch (wd) {
	case 1:
		*dwidth = DL_DMA_WIDTH_BYTE;
		break;
	case 2:
		*dwidth = DMA_DMACTL_DMASRCWDTH_HALF;
		break;
	case 3:
		*dwidth = DMA_DMACTL_DMASRCWDTH_WORD;
		break;
	case 4:
		*dwidth = DMA_DMACTL_DMASRCWDTH_LONG;
		break;
	default:
		*dwidth = DL_DMA_WIDTH_BYTE;
		break;
	}

	return 0;
}

static int dma_mspm0_configure(const struct device *dev, uint32_t channel,
			       struct dma_config *config)
{
	uint32_t temp;
	const struct dma_mspm0_config *cfg = dev->config;
	struct dma_mspm0_data *dma_data = dev->data;
	struct dma_mspm0_channel_data *data = NULL;
	struct dma_block_config *b_cfg = config->head_block;
	DL_DMA_Config dma_cfg = {0};

	if ((config == NULL) || (channel > DMA_MAX_CHANNEL)) {
		return -EINVAL;
	}

	data = &dma_data->ch_data[channel];
	if (data->busy != false) {
		return -EBUSY;
	}

	if (config->dest_data_size != config->source_data_size) {
		return -EINVAL;
	}

	if (mspm0_get_memory_increment(b_cfg->source_addr_adj, &temp)){
		return -EINVAL;
	}

	dma_cfg.srcIncrement = temp;
	if (mspm0_get_memory_increment(b_cfg->dest_addr_adj, &temp)){
		return -EINVAL;
	}

	dma_cfg.destIncrement = temp;
	if (mspm0_get_datawidth(config->dest_data_size, &temp)) {
		return -EINVAL;
	}

	dma_cfg.destWidth      = temp;
	if (mspm0_get_datawidth(config->source_data_size, &temp)) {
		return -EINVAL;
	}

	dma_cfg.srcWidth	= temp ;
	data->direction		= config->channel_direction;
	data->dma_callback	= config->dma_callback;
	data->user_data		= config->user_data;
	dma_cfg.transferMode	= DL_DMA_SINGLE_TRANSFER_MODE,
	dma_cfg.extendedMode	= DL_DMA_NORMAL_MODE,
	dma_cfg.triggerType	= DL_DMA_TRIGGER_TYPE_EXTERNAL;
	dma_cfg.trigger		= config->dma_slot;

	DL_DMA_clearInterruptStatus(DMA, channel + 1);
	DL_DMA_enableInterrupt(DMA, channel + 1);
	DL_DMA_setTransferSize(cfg->base, channel, b_cfg->block_size);
	DL_DMA_initChannel(cfg->base, channel, &dma_cfg);
	DL_DMA_setSrcAddr(cfg->base, channel, b_cfg->source_address);
	DL_DMA_setDestAddr(cfg->base, channel, b_cfg->dest_address);
	data->busy = true;

	return 0;
}

static int dma_mspm0_start(const struct device *dev, const uint32_t channel)
{
	const struct dma_mspm0_config *cfg = dev->config;

	if (channel > DMA_MAX_CHANNEL) {
		return -EINVAL;
	}

	DL_DMA_enableChannel(cfg->base, channel);

	return 0;
}

static int dma_mspm0_stop(const struct device *dev, const uint32_t channel)
{
	const struct dma_mspm0_config *cfg = dev->config;
	struct dma_mspm0_data *data = dev->data;

	if (channel > DMA_MAX_CHANNEL) {
		return -EINVAL;
	}

	DL_DMA_disableChannel(cfg->base, channel);
	data->ch_data[channel].busy = false;

	return 0;
}

static int dma_mspm0_reload(const struct device *dev, uint32_t channel, uint32_t src_addr,
			    uint32_t dest_addr, size_t size)
{
	const struct dma_mspm0_config *cfg = dev->config;
	struct dma_mspm0_channel_data *data = NULL;
	struct dma_mspm0_data *dma_data = dev->data;

	if (channel > DMA_MAX_CHANNEL) {
		return -EINVAL;
	}

	data = &dma_data->ch_data[channel];
	switch (data->direction) {
	case PERIPHERAL_TO_MEMORY:
		DL_DMA_setDestAddr(cfg->base, channel, dest_addr);
		break;
	case MEMORY_TO_PERIPHERAL:
		DL_DMA_setSrcAddr(cfg->base, channel, src_addr);
		break;
	default:
		return -ENOTSUP;
	}

	DL_DMA_setTransferSize(cfg->base, channel, size);
	data->busy = true;

	return 0;
}

static int dma_mspm0_get_status(const struct device *dev, uint32_t channel,
				struct dma_status *stat)
{
	const struct dma_mspm0_config *cfg = dev->config;
	struct dma_mspm0_data *dma_data = dev->data;
	struct dma_mspm0_channel_data *data;

	if (channel > DMA_MAX_CHANNEL) {
		return -EINVAL;
	}

	data = &dma_data->ch_data[channel];
	stat->pending_length = DL_DMA_getTransferSize(cfg->base, channel);
	stat->dir = data->direction;
	stat->busy = data->busy;

	return 0;
}

static void dma_mspm0_isr(const struct device *dev)
{
	uint8_t interrupt_mask;
	uint8_t channel;
	uint32_t key = irq_lock();
	const struct dma_mspm0_config *cfg = dev->config;
	struct dma_mspm0_data *dma_data = dev->data;
	struct dma_mspm0_channel_data *data;

	interrupt_mask = DL_DMA_getPendingInterrupt(cfg->base);
	switch(interrupt_mask) {
	case DL_DMA_EVENT_IIDX_DMACH0:
		data = &dma_data->ch_data[0];
		channel = 0;
		break;
	case DL_DMA_EVENT_IIDX_DMACH1:
		data = &dma_data->ch_data[1];
		channel = 1;
		break;
	case DL_DMA_EVENT_IIDX_DMACH2:
		data = &dma_data->ch_data[2];
		channel = 2;
		break;
	case DL_DMA_EVENT_IIDX_DMACH3:
		data = &dma_data->ch_data[3];
		channel = 3;
		break;
	case DL_DMA_EVENT_IIDX_DMACH4:
		data = &dma_data->ch_data[4];
		channel = 4;
		break;
	case DL_DMA_EVENT_IIDX_DMACH5:
		data = &dma_data->ch_data[5];
		channel = 5;
		break;
	case DL_DMA_EVENT_IIDX_DMACH6:
		data = &dma_data->ch_data[6];
		channel = 6;
		break;
	default:
		irq_unlock(key);
		DL_DMA_clearInterruptStatus(cfg->base, interrupt_mask);
		DL_DMA_clearEventsStatus(cfg->base, interrupt_mask);
		return;
	}

	DL_DMA_clearInterruptStatus(cfg->base, interrupt_mask);
	DL_DMA_clearEventsStatus(cfg->base, interrupt_mask);

	DL_DMA_disableChannel(cfg->base, channel);
	if (data->dma_callback) {
		data->busy = false;
		data->dma_callback(dev, data->user_data, channel, 0);
	}

	irq_unlock(key);
}

static inline void dma_mspm0_irq_config(const struct device *dev)
{
	irq_disable(DT_INST_IRQN(0));
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
		    dma_mspm0_isr, DEVICE_DT_INST_GET(0), 0);

	irq_enable(DT_INST_IRQN(0));
}

static int mspm0_dma_init(const struct device *dev)
{
	dma_mspm0_irq_config(dev);

	return 0;
};

static DEVICE_API(dma, dma_mspm0_api) = {
	.config		= dma_mspm0_configure,
	.start		= dma_mspm0_start,
	.stop		= dma_mspm0_stop,
	.reload		= dma_mspm0_reload,
	.get_status	= dma_mspm0_get_status,
};

#define MSPM0_DMA_INIT(inst)                                                                    \
	static const struct dma_mspm0_config dma_cfg_##inst = {                                 \
		.base = (DMA_Regs *)DT_INST_REG_ADDR(inst),                                      \
	};                                                                                      \
	struct dma_mspm0_data dma_data_##inst;							\
	DEVICE_DT_INST_DEFINE(inst, &mspm0_dma_init, NULL, &dma_data_##inst, &dma_cfg_##inst,	\
			      PRE_KERNEL_1, CONFIG_DMA_INIT_PRIORITY, &dma_mspm0_api);

DT_INST_FOREACH_STATUS_OKAY(MSPM0_DMA_INIT)
