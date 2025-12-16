/*
 * Copyright (c) 2025-2026 Conclusive Engineering Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc35xx_dma

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dma_cc45xx, CONFIG_DMA_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/irq.h>
#include <zephyr/dt-bindings/dma/ti-cc35xx-dma.h>

#include <ti/devices/cc35xx/driverlib/dma.h>

#include <inc/hw_memmap.h>

#define DMA_INT_ALL SOC_AAON_DMANSICLR_ICLR_M

struct dma_cc35xx_config {
	int src_block_size;
};

struct dma_cc35xx_channel {
	uint32_t *dst;
	uint32_t *src;
	uint16_t len;
	dma_callback_t cb;
	void *user_data;
};

struct dma_cc35xx_data {
	struct dma_context ctx;
	struct dma_cc35xx_channel *channels;
	uint32_t channel_control;
};

static inline void dma_cc35xx_int_disable(uint32_t mask)
{
	mem_addr_t addr = SOC_AAON_BASE + SOC_AAON_O_DMANSIMASK;

	sys_write32(sys_read32(addr) & ~mask, addr);
}

static inline void dma_cc35xx_int_enable(uint32_t mask)
{
	mem_addr_t addr = SOC_AAON_BASE + SOC_AAON_O_DMANSIMASK;

	sys_write32(sys_read32(addr) | mask, addr);
}

static inline uint32_t dma_cc35xx_int_status(void)
{
	return sys_read32(SOC_AAON_BASE + SOC_AAON_O_DMANSMIS);
}

static inline void dma_cc35xx_int_clear(uint32_t mask)
{
	sys_write32(mask, SOC_AAON_BASE + SOC_AAON_O_DMANSICLR);
}

static inline uint32_t dma_cc35xx_get_channel_config_reg(uint32_t channel)
{
	return sys_read32(HOST_DMA_TGT_BASE + HOST_DMA_O_CH0JCTL + (channel * DMA_CH_OFFSET));
}

static inline uint32_t dma_cc35xx_get_channel_status_reg(uint32_t channel)
{
	return sys_read32(HOST_DMA_TGT_BASE + HOST_DMA_O_CH0TSTA + (channel * DMA_CH_OFFSET));
}

static inline uint32_t dma_cc35xx_pending_length(uint32_t channel)
{
	volatile int timeout = 5000;

	/* Wait for the correct value of pending length after aborted transaction */
	while (timeout--) {
	};

	return (dma_cc35xx_get_channel_status_reg(channel) & HOST_DMA_CH0TSTA_REMAINB_M) >>
	       HOST_DMA_CH0TSTA_REMAINB_S;
}

static void dma_cc35xx_disable_channel(uint32_t channel)
{
	uint32_t addr;

	addr = HOST_DMA_TGT_BASE + HOST_DMA_O_CH0TCTL2 + (channel * DMA_CH_OFFSET);
	sys_write32(DMA_CMD_ABORT, addr);

	/* Wait for ABORT command to finish.
	 * The command finishes within maximum eight clock cycles.
	 */
	addr = HOST_DMA_TGT_BASE + HOST_DMA_O_CH0STA + (channel * DMA_CH_OFFSET);

	while ((sys_read32(addr) & HOST_DMA_CH0STA_RUN_M) == HOST_DMA_CH0STA_RUN) {
	}
}

static void dma_cc35xx_init_channel(uint32_t channel)
{
	uint32_t addr;

	addr = HOST_DMA_TGT_BASE + HOST_DMA_O_CH0TCTL2 + (channel * DMA_CH_OFFSET);
	sys_write32(DMA_CMD_INIT, addr);
}

static int dma_cc35xx_config(const struct device *dev, uint32_t channel, struct dma_config *config)
{
	struct dma_cc35xx_data *data = dev->data;
	struct dma_cc35xx_channel *ch_data;
	int num_dma_channels = data->ctx.dma_channels;
	uint32_t dma_config, dma_word_size;
	uint8_t periph_index;

	if (channel >= num_dma_channels) {
		LOG_ERR("Channel %d is out of the range", channel);
		return -ERANGE;
	}

	if (!config || !config->head_block || config->block_count < 1) {
		return -EINVAL;
	}

	periph_index = CC35xx_DMA_GET_PERIPH_INDEX(config->dma_slot);
	dma_config = DMA_CONFIG_CLEAR_AT_JOB_START;

	if (!periph_index && ((config->channel_direction == MEMORY_TO_PERIPHERAL) ||
			      (config->channel_direction == PERIPHERAL_TO_MEMORY))) {
		LOG_ERR("Invalid configuration, peripheral not selected");
		return -EINVAL;
	}

	if (config->head_block->dest_addr_adj == DMA_ADDR_ADJ_NO_CHANGE) {
		dma_config |= DMA_CONFIG_DST_PTR_FIFO;
	}

	if (config->head_block->source_addr_adj == DMA_ADDR_ADJ_NO_CHANGE) {
		dma_config |= DMA_CONFIG_SRC_PTR_FIFO;
	}

	switch (config->channel_direction) {
	case MEMORY_TO_MEMORY:
		dma_config |= DMA_CONFIG_FORCE_REQ;
		break;
	case MEMORY_TO_PERIPHERAL:
		dma_config |= DMA_CONFIG_TX;
		break;
	case PERIPHERAL_TO_MEMORY:
		dma_config |= DMA_CONFIG_RX;
		break;
	default:
		LOG_ERR("Unsupported channel direction");
		return -EINVAL;
	}

	if (config->dest_data_size != 1 && config->dest_data_size != 2 &&
	    config->dest_data_size != 4) {
		LOG_ERR("Invalid dest size, Only 1,2,4 bytes supported");
		return -EINVAL;
	}

	if (config->source_data_size != 1 && config->source_data_size != 2 &&
	    config->source_data_size != 4) {
		LOG_ERR("Invalid source size, Only 1,2,4 bytes supported");
		return -EINVAL;
	}

	dma_word_size = DMA_WORD_SIZE_1B - __builtin_ctz((config->source_data_size));

	ch_data = &data->channels[channel];

	ch_data->src = (uint32_t *)config->head_block->source_address;
	ch_data->dst = (uint32_t *)config->head_block->dest_address;
	ch_data->len = config->head_block->block_size;
	ch_data->cb = config->dma_callback;
	ch_data->user_data = config->user_data;
	data->channel_control |= BIT(channel);

	if (periph_index) {
		DMAInitChannel(channel, periph_index - 1);
	}
	DMAConfigureChannel(channel, config->source_burst_length, dma_word_size, dma_config);

	return 0;
}

static int dma_cc35xx_start(const struct device *dev, uint32_t channel)
{
	struct dma_cc35xx_data *data = dev->data;
	struct dma_cc35xx_channel *ch_data;
	int num_dma_channels = data->ctx.dma_channels;

	if (channel >= num_dma_channels) {
		return -EINVAL;
	}

	dma_cc35xx_init_channel(channel);
	ch_data = &data->channels[channel];
	DMAStartTransaction(channel, ch_data->src, ch_data->dst, ch_data->len, true);

	dma_cc35xx_int_enable(BIT(channel));

	return 0;
}

static int dma_cc35xx_stop(const struct device *dev, uint32_t channel)
{
	struct dma_cc35xx_data *data = dev->data;
	int num_dma_channels = data->ctx.dma_channels;

	if (channel >= num_dma_channels) {
		return -EINVAL;
	}

	dma_cc35xx_int_disable(BIT(channel));
	dma_cc35xx_int_clear(BIT(channel));

	dma_cc35xx_disable_channel(channel);

	return 0;
}

static int dma_cc35xx_reload(const struct device *dev, uint32_t channel, uint32_t src, uint32_t dst,
			     size_t size)
{
	struct dma_cc35xx_data *data = dev->data;
	int num_dma_channels = data->ctx.dma_channels;

	if (channel >= num_dma_channels) {
		return -EINVAL;
	}

	dma_cc35xx_init_channel(channel);

	DMAStartTransaction(channel, (uint32_t *)src, (uint32_t *)dst, size, true);
	dma_cc35xx_int_enable(BIT(channel));

	return 0;
}

static int dma_cc35xx_get_status(const struct device *dev, uint32_t channel,
				 struct dma_status *stat)
{
	struct dma_cc35xx_data *data = dev->data;
	int num_dma_channels = data->ctx.dma_channels;
	uint32_t channel_config;

	if ((channel >= num_dma_channels) || !stat) {
		return -EINVAL;
	}
	channel_config = dma_cc35xx_get_channel_config_reg(channel);

	if (channel_config & DMA_CONFIG_FORCE_REQ) {
		stat->dir = MEMORY_TO_MEMORY;
	} else if (DMAGetChannelDirection(channel) == DMA_CONFIG_TX) {
		stat->dir = MEMORY_TO_PERIPHERAL;
	} else {
		stat->dir = PERIPHERAL_TO_MEMORY;
	}

	stat->busy = (DMAGetChannelStatus(channel) == 0);
	stat->pending_length = dma_cc35xx_pending_length(channel);

	return 0;
}

static void dma_cc35xx_isr(const struct device *dev)
{
	struct dma_cc35xx_data *data = dev->data;
	struct dma_cc35xx_channel *ch_data;
	int num_dma_channels = data->ctx.dma_channels;
	uint32_t i, done_status = dma_cc35xx_int_status();

	for (i = 0; i < num_dma_channels; i++) {
		if (done_status & BIT(i)) {
			ch_data = &data->channels[i];
			if (ch_data->cb) {
				ch_data->cb(dev, ch_data->user_data, i, DMA_STATUS_COMPLETE);
			}

			dma_cc35xx_int_clear(done_status & BIT(i));
		}
	}
}

static void dma_CC35XX_channels_init(struct dma_cc35xx_data *data)
{
	int i;

	for (i = 0; i < data->ctx.dma_channels; i++) {
		DMAInitChannel(i, DMA_NUM_PERIPHS);
	}
}

static int dma_cc35xx_init(const struct device *dev)
{
	struct dma_cc35xx_data *data = dev->data;

	data->channel_control = 0;

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), dma_cc35xx_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));
	dma_CC35XX_channels_init(data);
	dma_cc35xx_int_disable(DMA_INT_ALL);
	dma_cc35xx_int_clear(DMA_INT_ALL);
	return 0;
}

static DEVICE_API(dma, dma_cc35xx_driver_api) = {
	.config = dma_cc35xx_config,
	.reload = dma_cc35xx_reload,
	.get_status = dma_cc35xx_get_status,
	.start = dma_cc35xx_start,
	.stop = dma_cc35xx_stop,
};

#define CC35XX_DMA_INIT(inst)                                                                      \
                                                                                                   \
	static struct dma_cc35xx_channel                                                           \
		dma_cc35xx##inst##_channels[DT_INST_PROP(inst, dma_channels)];                     \
	ATOMIC_DEFINE(dma_cc35xx_atomic##inst, DT_INST_PROP(inst, dma_channels));                  \
	static struct dma_cc35xx_data dma_cc35xx##inst##_data = {                                  \
		.ctx =                                                                             \
			{                                                                          \
				.magic = DMA_MAGIC,                                                \
				.atomic = dma_cc35xx_atomic##inst,                                 \
				.dma_channels = DT_INST_PROP(inst, dma_channels),                  \
			},                                                                         \
		.channels = dma_cc35xx##inst##_channels,                                           \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, &dma_cc35xx_init, NULL, &dma_cc35xx##inst##_data,              \
			      &dma_cc35xx_config, PRE_KERNEL_1, CONFIG_DMA_INIT_PRIORITY,          \
			      &dma_cc35xx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CC35XX_DMA_INIT)
