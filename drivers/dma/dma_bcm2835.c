/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT brcm_bcm2835_dma

#include <errno.h>
#include <stdint.h>

#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(dma_bcm2835, CONFIG_DMA_LOG_LEVEL);

#define BCM2835_DMA_CHAN_SIZE 0x100U

#define BCM2835_DMA_CS        0x00U
#define BCM2835_DMA_ADDR      0x04U
#define BCM2835_DMA_TI        0x08U
#define BCM2835_DMA_SOURCE_AD 0x0cU
#define BCM2835_DMA_DEST_AD   0x10U
#define BCM2835_DMA_LEN       0x14U
#define BCM2835_DMA_NEXTCB    0x1cU
#define BCM2835_DMA_DEBUG     0x20U

#define BCM2835_DMA_INT_STATUS 0xfe0U
#define BCM2835_DMA_ENABLE     0xff0U

#define BCM2835_DMA_CS_ACTIVE        BIT(0)
#define BCM2835_DMA_CS_END           BIT(1)
#define BCM2835_DMA_CS_INT           BIT(2)
#define BCM2835_DMA_CS_ERR           BIT(8)
#define BCM2835_DMA_CS_PRIORITY(n)   (((n) & 0xfU) << 16)
#define BCM2835_DMA_CS_PANIC_PRIO(n) (((n) & 0xfU) << 20)
#define BCM2835_DMA_CS_WAIT_WRITES   BIT(28)
#define BCM2835_DMA_CS_DIS_DEBUG     BIT(29)
#define BCM2835_DMA_CS_ABORT         BIT(30)
#define BCM2835_DMA_CS_RESET         BIT(31)

#define BCM2835_DMA_TI_INT_EN         BIT(0)
#define BCM2835_DMA_TI_WAIT_RESP      BIT(3)
#define BCM2835_DMA_TI_DEST_INC       BIT(4)
#define BCM2835_DMA_TI_SRC_INC        BIT(8)
#define BCM2835_DMA_TI_NO_WIDE_BURSTS BIT(26)

#define BCM2835_DMA_DEBUG_ERR_MASK (BIT(0) | BIT(1) | BIT(2))

#define BCM2835_DMA_BUS_ALIAS 0x40000000U
#define BCM2835_DMA_CB_ALIGN  32U
#define BCM2835_DMA_UNIQUE_IRQS 12

struct bcm2835_dma_cb {
	uint32_t ti;
	uint32_t source_ad;
	uint32_t dest_ad;
	uint32_t txfr_len;
	uint32_t stride;
	uint32_t nextconbk;
	uint32_t reserved[2];
} __aligned(BCM2835_DMA_CB_ALIGN);

struct bcm2835_dma_channel {
	struct bcm2835_dma_cb cb;
	struct dma_config dma_cfg;
	struct dma_block_config block_cfg;
	bool configured;
	bool busy;
};

struct bcm2835_dma_config {
	uintptr_t base;
	uint32_t channel_mask;
	uint32_t dma_channels;
};

struct bcm2835_dma_data {
	struct dma_context ctx;
	atomic_t *atomic;
	struct bcm2835_dma_channel channels[CONFIG_DMA_BCM2835_MAX_CHANNELS];
};

static uintptr_t bcm2835_dma_chan_base(const struct device *dev, uint32_t channel)
{
	const struct bcm2835_dma_config *cfg = dev->config;

	return cfg->base + (channel * BCM2835_DMA_CHAN_SIZE);
}

static uint32_t bcm2835_dma_bus_addr(uintptr_t addr)
{
	return (uint32_t)addr + BCM2835_DMA_BUS_ALIAS;
}

static bool bcm2835_dma_channel_valid(const struct device *dev, uint32_t channel)
{
	const struct bcm2835_dma_config *cfg = dev->config;

	return channel < cfg->dma_channels &&
	       channel < CONFIG_DMA_BCM2835_MAX_CHANNELS &&
	       (cfg->channel_mask & BIT(channel)) != 0U;
}

static void bcm2835_dma_reset_chan(const struct device *dev, uint32_t channel)
{
	uintptr_t chan_base = bcm2835_dma_chan_base(dev, channel);

	sys_write32(BCM2835_DMA_CS_RESET, chan_base + BCM2835_DMA_CS);
	sys_write32(BCM2835_DMA_DEBUG_ERR_MASK, chan_base + BCM2835_DMA_DEBUG);
}

static int bcm2835_dma_configure(const struct device *dev, uint32_t channel,
				 struct dma_config *dma_cfg)
{
	struct bcm2835_dma_data *data = dev->data;
	struct bcm2835_dma_channel *chan;
	struct dma_block_config *block;

	if (!bcm2835_dma_channel_valid(dev, channel)) {
		return -EINVAL;
	}

	if (dma_cfg->channel_direction != MEMORY_TO_MEMORY ||
	    dma_cfg->block_count != 1U || dma_cfg->head_block == NULL ||
	    dma_cfg->cyclic || dma_cfg->source_chaining_en ||
	    dma_cfg->dest_chaining_en) {
		return -ENOTSUP;
	}

	block = dma_cfg->head_block;
	if (block->block_size == 0U || block->block_size > UINT16_MAX ||
	    block->source_gather_en || block->dest_scatter_en ||
	    block->source_addr_adj != DMA_ADDR_ADJ_INCREMENT ||
	    block->dest_addr_adj != DMA_ADDR_ADJ_INCREMENT) {
		return -ENOTSUP;
	}

	chan = &data->channels[channel];
	chan->dma_cfg = *dma_cfg;
	chan->block_cfg = *block;
	chan->dma_cfg.head_block = &chan->block_cfg;
	chan->configured = true;
	chan->busy = false;

	return 0;
}

static int bcm2835_dma_reload(const struct device *dev, uint32_t channel,
			      uint32_t src, uint32_t dst, size_t size)
{
	struct bcm2835_dma_data *data = dev->data;
	struct bcm2835_dma_channel *chan;

	if (!bcm2835_dma_channel_valid(dev, channel)) {
		return -EINVAL;
	}

	if (size == 0U || size > UINT16_MAX) {
		return -EINVAL;
	}

	chan = &data->channels[channel];
	if (!chan->configured || chan->busy) {
		return -EINVAL;
	}

	chan->block_cfg.source_address = src;
	chan->block_cfg.dest_address = dst;
	chan->block_cfg.block_size = size;

	return 0;
}

static int bcm2835_dma_start(const struct device *dev, uint32_t channel)
{
	const struct bcm2835_dma_config *cfg = dev->config;
	struct bcm2835_dma_data *data = dev->data;
	struct bcm2835_dma_channel *chan;
	struct dma_block_config *block;
	uintptr_t chan_base;
	uint32_t ti;

	if (!bcm2835_dma_channel_valid(dev, channel)) {
		return -EINVAL;
	}

	chan = &data->channels[channel];
	if (!chan->configured || chan->busy) {
		return -EINVAL;
	}

	block = &chan->block_cfg;
	ti = BCM2835_DMA_TI_WAIT_RESP | BCM2835_DMA_TI_SRC_INC |
	     BCM2835_DMA_TI_DEST_INC | BCM2835_DMA_TI_NO_WIDE_BURSTS;

	if (chan->dma_cfg.dma_callback != NULL) {
		ti |= BCM2835_DMA_TI_INT_EN;
	}

	chan->cb.ti = ti;
	chan->cb.source_ad = bcm2835_dma_bus_addr((uintptr_t)block->source_address);
	chan->cb.dest_ad = bcm2835_dma_bus_addr((uintptr_t)block->dest_address);
	chan->cb.txfr_len = block->block_size;
	chan->cb.stride = 0U;
	chan->cb.nextconbk = 0U;
	chan->cb.reserved[0] = 0U;
	chan->cb.reserved[1] = 0U;

	(void)sys_cache_data_flush_range((void *)block->source_address, block->block_size);
	(void)sys_cache_data_flush_range(&chan->cb, sizeof(chan->cb));

	chan->busy = true;
	chan_base = bcm2835_dma_chan_base(dev, channel);

	sys_write32(sys_read32(cfg->base + BCM2835_DMA_ENABLE) | BIT(channel),
		    cfg->base + BCM2835_DMA_ENABLE);
	sys_write32(BCM2835_DMA_CS_RESET, chan_base + BCM2835_DMA_CS);
	sys_write32(BCM2835_DMA_DEBUG_ERR_MASK, chan_base + BCM2835_DMA_DEBUG);
	sys_write32(bcm2835_dma_bus_addr((uintptr_t)&chan->cb), chan_base + BCM2835_DMA_ADDR);
	sys_write32(BCM2835_DMA_CS_ACTIVE | BCM2835_DMA_CS_PRIORITY(1) |
		    BCM2835_DMA_CS_PANIC_PRIO(1) | BCM2835_DMA_CS_WAIT_WRITES |
		    BCM2835_DMA_CS_DIS_DEBUG,
		    chan_base + BCM2835_DMA_CS);

	return 0;
}

static int bcm2835_dma_stop(const struct device *dev, uint32_t channel)
{
	struct bcm2835_dma_data *data = dev->data;
	uintptr_t chan_base;

	if (!bcm2835_dma_channel_valid(dev, channel)) {
		return -EINVAL;
	}

	chan_base = bcm2835_dma_chan_base(dev, channel);
	sys_write32(BCM2835_DMA_CS_ABORT | BCM2835_DMA_CS_ACTIVE, chan_base + BCM2835_DMA_CS);
	sys_write32(BCM2835_DMA_CS_RESET, chan_base + BCM2835_DMA_CS);
	data->channels[channel].busy = false;

	return 0;
}

static int bcm2835_dma_get_status(const struct device *dev, uint32_t channel,
				  struct dma_status *status)
{
	struct bcm2835_dma_data *data = dev->data;
	uint32_t cs;

	if (!bcm2835_dma_channel_valid(dev, channel)) {
		return -EINVAL;
	}

	cs = sys_read32(bcm2835_dma_chan_base(dev, channel) + BCM2835_DMA_CS);
	status->busy = data->channels[channel].busy || ((cs & BCM2835_DMA_CS_ACTIVE) != 0U);
	status->dir = MEMORY_TO_MEMORY;
	status->pending_length = 0U;

	return 0;
}

static void bcm2835_dma_isr(const struct device *dev)
{
	const struct bcm2835_dma_config *cfg = dev->config;
	struct bcm2835_dma_data *data = dev->data;
	uint32_t pending = sys_read32(cfg->base + BCM2835_DMA_INT_STATUS);

	for (uint32_t channel = 0U; channel < cfg->dma_channels; channel++) {
		struct bcm2835_dma_channel *chan;
		uintptr_t chan_base;
		uint32_t cs;
		int status = DMA_STATUS_COMPLETE;

		if ((pending & BIT(channel)) == 0U || !bcm2835_dma_channel_valid(dev, channel)) {
			continue;
		}

		chan = &data->channels[channel];
		chan_base = bcm2835_dma_chan_base(dev, channel);
		cs = sys_read32(chan_base + BCM2835_DMA_CS);

		if ((cs & BCM2835_DMA_CS_ERR) != 0U) {
			status = -EIO;
		}

		sys_write32(BCM2835_DMA_CS_INT | BCM2835_DMA_CS_ACTIVE |
			    BCM2835_DMA_CS_PRIORITY(1) | BCM2835_DMA_CS_PANIC_PRIO(1) |
			    BCM2835_DMA_CS_WAIT_WRITES | BCM2835_DMA_CS_DIS_DEBUG,
			    chan_base + BCM2835_DMA_CS);

		if (sys_read32(chan_base + BCM2835_DMA_ADDR) == 0U || status < 0) {
			chan->busy = false;
			if (chan->dma_cfg.dma_callback != NULL) {
				chan->dma_cfg.dma_callback(dev, chan->dma_cfg.user_data,
							   channel, status);
			}
		}
	}
}

static bool bcm2835_dma_chan_filter(const struct device *dev, int channel,
				    void *filter_param)
{
	ARG_UNUSED(filter_param);

	return bcm2835_dma_channel_valid(dev, channel);
}

static int bcm2835_dma_init(const struct device *dev)
{
	const struct bcm2835_dma_config *cfg = dev->config;

	for (uint32_t channel = 0U; channel < cfg->dma_channels; channel++) {
		if (bcm2835_dma_channel_valid(dev, channel)) {
			bcm2835_dma_reset_chan(dev, channel);
		}
	}

	return 0;
}

static DEVICE_API(dma, bcm2835_dma_api) = {
	.config = bcm2835_dma_configure,
	.reload = bcm2835_dma_reload,
	.start = bcm2835_dma_start,
	.stop = bcm2835_dma_stop,
	.get_status = bcm2835_dma_get_status,
	.chan_filter = bcm2835_dma_chan_filter,
};

#define BCM2835_DMA_IRQ_CONNECT(idx, inst)                                               \
	do {                                                                             \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(inst, idx, irq), 0, bcm2835_dma_isr,      \
			    DEVICE_DT_INST_GET(inst), 0);                                \
		irq_enable(DT_INST_IRQ_BY_IDX(inst, idx, irq));                         \
	} while (false)

#define BCM2835_DMA_INIT(inst)                                                           \
	ATOMIC_DEFINE(bcm2835_dma_atomic_##inst, DT_INST_PROP(inst, dma_channels));      \
	static struct bcm2835_dma_data bcm2835_dma_data_##inst = {                       \
		.ctx = {                                                                 \
			.magic = DMA_MAGIC,                                              \
			.atomic = bcm2835_dma_atomic_##inst,                             \
			.dma_channels = DT_INST_PROP(inst, dma_channels),                \
		},                                                                       \
		.atomic = bcm2835_dma_atomic_##inst,                                      \
	};                                                                               \
	static const struct bcm2835_dma_config bcm2835_dma_config_##inst = {             \
		.base = DT_INST_REG_ADDR(inst),                                          \
		.channel_mask = DT_INST_PROP(inst, brcm_dma_channel_mask),               \
		.dma_channels = DT_INST_PROP(inst, dma_channels),                        \
	};                                                                               \
	static int bcm2835_dma_init_##inst(const struct device *dev)                     \
	{                                                                                \
		int ret;                                                                 \
		                                                                         \
		ret = bcm2835_dma_init(dev);                                             \
		if (ret != 0) {                                                          \
			return ret;                                                      \
		}                                                                        \
		/* Channels 11-14 share the same IRQ line; connect each unique line once. */ \
		LISTIFY(BCM2835_DMA_UNIQUE_IRQS, BCM2835_DMA_IRQ_CONNECT, (;), inst);    \
		return 0;                                                                \
	}                                                                                \
	DEVICE_DT_INST_DEFINE(inst, bcm2835_dma_init_##inst, NULL,                      \
			      &bcm2835_dma_data_##inst, &bcm2835_dma_config_##inst,     \
			      PRE_KERNEL_1, CONFIG_DMA_INIT_PRIORITY, &bcm2835_dma_api);

DT_INST_FOREACH_STATUS_OKAY(BCM2835_DMA_INIT)
