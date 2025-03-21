/*
 * Copyright (c) 2024 Paul Wedeck <paulwedeck@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT wch_wch_dma

#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/clock_control.h>

#include <ch32fun.h>

#define DMA_WCH_MAX_CHAN      11
#define DMA_WCH_MAX_CHAN_BASE 8

#define DMA_WCH_AIF        (DMA_GIF1 | DMA_TCIF1 | DMA_HTIF1 | DMA_TEIF1)
#define DMA_WCH_IF_OFF(ch) (4 * (ch))
#define DMA_WCH_MAX_BLOCK  ((UINT32_C(2) << 16) - 1)

struct dma_wch_chan_regs {
	volatile uint32_t CFGR;
	volatile uint32_t CNTR;
	volatile uint32_t PADDR;
	volatile uint32_t MADDR;
	volatile uint32_t reserved1;
};

struct dma_wch_regs {
	DMA_TypeDef base;
	struct dma_wch_chan_regs channels[DMA_WCH_MAX_CHAN];
	DMA_TypeDef ext;
};

struct dma_wch_config {
	struct dma_wch_regs *regs;
	uint32_t num_channels;
	const struct device *clock_dev;
	uint8_t clock_id;
	void (*irq_config_func)(const struct device *dev);
};

struct dma_wch_channel {
	void *user_data;
	dma_callback_t dma_cb;
};

struct dma_wch_data {
	struct dma_context ctx;
	struct dma_wch_channel *channels;
};

static uint8_t dma_wch_get_ip(const struct device *dev, uint32_t chan)
{
	const struct dma_wch_config *config = dev->config;
	struct dma_wch_regs *regs = config->regs;
	uint32_t intfr;

	if (chan > DMA_WCH_MAX_CHAN_BASE) {
		chan -= DMA_WCH_MAX_CHAN_BASE;
		intfr = regs->ext.INTFR;
		return (intfr >> DMA_WCH_IF_OFF(chan)) & DMA_WCH_AIF;
	}

	intfr = regs->base.INTFR;
	return (intfr >> DMA_WCH_IF_OFF(chan)) & DMA_WCH_AIF;
}

static bool dma_wch_busy(const struct device *dev, uint32_t ch)
{
	const struct dma_wch_config *config = dev->config;
	struct dma_wch_regs *regs = config->regs;

	return (regs->channels[ch].CFGR & DMA_CFGR1_EN) > 0 &&
	       !(dma_wch_get_ip(dev, ch) & DMA_TCIF1);
}

static int dma_wch_init(const struct device *dev)
{
	const struct dma_wch_config *config = dev->config;
	clock_control_subsys_t clock_sys = (clock_control_subsys_t *)(uintptr_t)config->clock_id;

	if (config->num_channels > DMA_WCH_MAX_CHAN) {
		return -ENOTSUP;
	}

	clock_control_on(config->clock_dev, clock_sys);

	config->irq_config_func(dev);
	return 0;
}

static int dma_wch_config(const struct device *dev, uint32_t ch, struct dma_config *dma_cfg)
{
	const struct dma_wch_config *config = dev->config;
	struct dma_wch_data *data = dev->data;
	struct dma_wch_regs *regs = config->regs;
	unsigned int key;
	int ret = 0;

	uint32_t cfgr = 0;
	uint32_t cntr = 0;
	uint32_t paddr = 0;
	uint32_t maddr = 0;

	if (config->num_channels <= ch) {
		return -EINVAL;
	}

	if (dma_cfg->block_count != 1) {
		return -ENOTSUP;
	}

	if (dma_cfg->head_block->block_size > DMA_WCH_MAX_BLOCK ||
	    dma_cfg->head_block->source_gather_en || dma_cfg->head_block->dest_scatter_en ||
	    dma_cfg->head_block->source_reload_en || dma_cfg->channel_priority > 3 ||
	    dma_cfg->head_block->source_addr_adj == DMA_ADDR_ADJ_DECREMENT ||
	    dma_cfg->head_block->dest_addr_adj == DMA_ADDR_ADJ_DECREMENT ||
	    dma_cfg->head_block->dest_reload_en) {
		return -ENOTSUP;
	}

	cntr = dma_cfg->head_block->block_size;

	switch (dma_cfg->channel_direction) {
	case MEMORY_TO_MEMORY:
		cfgr |= DMA_CFGR1_MEM2MEM;
		paddr = dma_cfg->head_block->source_address;
		maddr = dma_cfg->head_block->dest_address;

		if (dma_cfg->cyclic) {
			return -ENOTSUP;
		}
		break;
	case MEMORY_TO_PERIPHERAL:
		maddr = dma_cfg->head_block->source_address;
		paddr = dma_cfg->head_block->dest_address;
		cfgr |= DMA_CFGR1_DIR;
		break;
	case PERIPHERAL_TO_MEMORY:
		paddr = dma_cfg->head_block->source_address;
		maddr = dma_cfg->head_block->dest_address;
		break;
	default:
		return -ENOTSUP;
	}
	cfgr |= dma_cfg->channel_priority * DMA_CFGR1_PL_0;

	if (dma_cfg->channel_direction == MEMORY_TO_PERIPHERAL) {
		cfgr |= dma_width_index(dma_cfg->source_data_size / BITS_PER_BYTE) *
			DMA_CFGR1_MSIZE_0;
		cfgr |= dma_width_index(dma_cfg->dest_data_size / BITS_PER_BYTE) *
			DMA_CFGR1_PSIZE_0;

		cfgr |= (dma_cfg->head_block->dest_addr_adj == DMA_ADDR_ADJ_INCREMENT)
				? DMA_CFGR1_PINC
				: 0;
		cfgr |= (dma_cfg->head_block->source_addr_adj == DMA_ADDR_ADJ_INCREMENT)
				? DMA_CFGR1_MINC
				: 0;
	} else {
		cfgr |= dma_width_index(dma_cfg->source_data_size / BITS_PER_BYTE) *
			DMA_CFGR1_PSIZE_0;
		cfgr |= dma_width_index(dma_cfg->dest_data_size / BITS_PER_BYTE) *
			DMA_CFGR1_MSIZE_0;

		cfgr |= (dma_cfg->head_block->dest_addr_adj == DMA_ADDR_ADJ_INCREMENT)
				? DMA_CFGR1_MINC
				: 0;
		cfgr |= (dma_cfg->head_block->source_addr_adj == DMA_ADDR_ADJ_INCREMENT)
				? DMA_CFGR1_PINC
				: 0;
	}

	if (dma_cfg->cyclic) {
		cfgr |= DMA_CFGR1_CIRC;
	}

	if (dma_cfg->dma_callback) {
		if (!dma_cfg->error_callback_dis) {
			cfgr |= DMA_CFGR1_TEIE;
		}

		if (dma_cfg->complete_callback_en) {
			cfgr |= DMA_CFGR1_HTIE;
		}

		cfgr |= DMA_CFGR1_TCIE;
	}

	key = irq_lock();

	if (dma_wch_busy(dev, ch)) {
		ret = -EBUSY;
		goto end;
	}

	data->channels[ch].user_data = dma_cfg->user_data;
	data->channels[ch].dma_cb = dma_cfg->dma_callback;

	regs->channels[ch].CFGR = 0;

	if (ch <= DMA_WCH_MAX_CHAN_BASE) {
		regs->base.INTFCR = DMA_WCH_AIF << DMA_WCH_IF_OFF(ch);
	} else {
		regs->ext.INTFCR = DMA_WCH_AIF << DMA_WCH_IF_OFF(ch - DMA_WCH_MAX_CHAN_BASE);
	}

	regs->channels[ch].PADDR = paddr;
	regs->channels[ch].MADDR = maddr;
	regs->channels[ch].CNTR = cntr;
	regs->channels[ch].CFGR = cfgr;
end:
	irq_unlock(key);
	return ret;
}

#ifdef CONFIG_DMA_64BIT
static int dma_wch_reload(const struct device *dev, uint32_t ch, uint64_t src, uint64_t dst,
			  size_t size)
#else
static int dma_wch_reload(const struct device *dev, uint32_t ch, uint32_t src, uint32_t dst,
			  size_t size)
#endif
{
	const struct dma_wch_config *config = dev->config;
	struct dma_wch_regs *regs = config->regs;
	uint32_t maddr, paddr;
	int ret = 0;
	unsigned int key;

	if (config->num_channels <= ch) {
		return -EINVAL;
	}

	key = irq_lock();

	if (dma_wch_busy(dev, ch)) {
		ret = -EBUSY;
		goto end;
	}

	if (regs->channels[ch].CFGR & DMA_CFGR1_DIR) {
		maddr = src;
		paddr = dst;
	} else {
		maddr = dst;
		paddr = src;
	}

	regs->channels[ch].MADDR = maddr;
	regs->channels[ch].PADDR = paddr;
	regs->channels[ch].CNTR = size;
end:
	irq_unlock(key);
	return ret;
}

static int dma_wch_start(const struct device *dev, uint32_t ch)
{
	const struct dma_wch_config *config = dev->config;
	struct dma_wch_regs *regs = config->regs;
	unsigned int key;

	if (config->num_channels <= ch) {
		return -EINVAL;
	}

	key = irq_lock();
	regs->channels[ch].CFGR |= DMA_CFGR1_EN;
	irq_unlock(key);

	return 0;
}

static int dma_wch_stop(const struct device *dev, uint32_t ch)
{
	const struct dma_wch_config *config = dev->config;
	struct dma_wch_regs *regs = config->regs;
	unsigned int key;

	if (config->num_channels <= ch) {
		return -EINVAL;
	}

	key = irq_lock();
	regs->channels[ch].CFGR &= ~DMA_CFGR1_EN;
	irq_unlock(key);

	return 0;
}

static int dma_wch_resume(const struct device *dev, uint32_t ch)
{
	const struct dma_wch_config *config = dev->config;
	struct dma_wch_regs *regs = config->regs;
	int ret = 0;
	unsigned int key;

	if (config->num_channels <= ch) {
		return -EINVAL;
	}

	key = irq_lock();

	if (regs->channels[ch].CFGR & DMA_CFGR1_EN) {
		ret = -EINVAL;
		goto end;
	}

	regs->channels[ch].CFGR |= DMA_CFGR1_EN;
end:
	irq_unlock(key);
	return ret;
}

static int dma_wch_suspend(const struct device *dev, uint32_t ch)
{
	const struct dma_wch_config *config = dev->config;
	struct dma_wch_regs *regs = config->regs;
	int ret = 0;
	unsigned int key;

	if (config->num_channels <= ch) {
		return -EINVAL;
	}

	key = irq_lock();

	if (!(regs->channels[ch].CFGR & DMA_CFGR1_EN)) {
		ret = -EINVAL;
		goto end;
	}

	regs->channels[ch].CFGR &= ~DMA_CFGR1_EN;
end:
	irq_unlock(key);
	return ret;
}

static int dma_wch_get_status(const struct device *dev, uint32_t ch, struct dma_status *status)
{
	const struct dma_wch_config *config = dev->config;
	struct dma_wch_regs *regs = config->regs;
	uint32_t cfgr;
	unsigned int key;

	if (config->num_channels <= ch) {
		return -EINVAL;
	}

	key = irq_lock();
	cfgr = regs->channels[ch].CFGR;

	status->busy = dma_wch_busy(dev, ch);
	if (cfgr & DMA_CFGR1_MEM2MEM) {
		status->dir = MEMORY_TO_MEMORY;
	} else if (cfgr & DMA_CFGR1_DIR) {
		status->dir = MEMORY_TO_PERIPHERAL;
	} else {
		status->dir = PERIPHERAL_TO_MEMORY;
	}

	status->pending_length = regs->channels[ch].CNTR;
	if (cfgr & DMA_CFGR1_DIR) {
		status->read_position = regs->channels[ch].MADDR;
		status->write_position = regs->channels[ch].PADDR;
	} else {
		status->read_position = regs->channels[ch].PADDR;
		status->write_position = regs->channels[ch].MADDR;
	}
	irq_unlock(key);
	return 0;
}

int dma_wch_get_attribute(const struct device *dev, uint32_t type, uint32_t *value)
{
	switch (type) {
	case DMA_ATTR_BUFFER_ADDRESS_ALIGNMENT:
	case DMA_ATTR_BUFFER_SIZE_ALIGNMENT:
	case DMA_ATTR_COPY_ALIGNMENT:
	case DMA_ATTR_MAX_BLOCK_COUNT:
		*value = 1;
		return 0;
	default:
		return -EINVAL;
	}
}

static void dma_wch_handle_callback(const struct device *dev, uint32_t ch, uint8_t ip)
{
	const struct dma_wch_data *data = dev->data;
	void *cb_data;
	dma_callback_t cb_func;
	unsigned int key = irq_lock();

	cb_data = data->channels[ch].user_data;
	cb_func = data->channels[ch].dma_cb;
	irq_unlock(key);

	if (!cb_func) {
		return;
	}

	if (ip & DMA_TCIF1) {
		cb_func(dev, cb_data, ch, DMA_STATUS_COMPLETE);
	} else if (ip & DMA_TEIF1) {
		cb_func(dev, cb_data, ch, -EIO);
	} else if (ip & DMA_HTIF1) {
		cb_func(dev, cb_data, ch, DMA_STATUS_BLOCK);
	}
}

static void dma_wch_isr(const struct device *dev, uint32_t chan)
{
	const struct dma_wch_config *config = dev->config;
	struct dma_wch_regs *regs = config->regs;
	uint32_t intfr = regs->base.INTFR;

	intfr &= (DMA_WCH_AIF << DMA_WCH_IF_OFF(chan));
	if (intfr & DMA_TCIF1 << DMA_WCH_IF_OFF(chan)) {
		regs->channels[chan].CFGR &= ~DMA_CFGR1_EN;
	}
	regs->base.INTFCR = intfr;

	dma_wch_handle_callback(dev, chan, intfr >> DMA_WCH_IF_OFF(chan));
}

static void dma_wch_isr_ext(const struct device *dev, uint32_t chan)
{
	const struct dma_wch_config *config = dev->config;
	struct dma_wch_regs *regs = config->regs;
	uint32_t chan_idx = chan - DMA_WCH_MAX_CHAN_BASE;
	uint32_t intfr = regs->ext.INTFR;

	intfr &= (DMA_WCH_AIF << DMA_WCH_IF_OFF(chan_idx));
	regs->ext.INTFCR = intfr;

	dma_wch_handle_callback(dev, chan, intfr >> DMA_WCH_IF_OFF(chan_idx));
}

static DEVICE_API(dma, dma_wch_driver_api) = {
	.config = dma_wch_config,
	.reload = dma_wch_reload,
	.start = dma_wch_start,
	.stop = dma_wch_stop,
	.resume = dma_wch_resume,
	.suspend = dma_wch_suspend,
	.get_status = dma_wch_get_status,
	.get_attribute = dma_wch_get_attribute,
};

#define GENERATE_ISR(ch, _)                                                                        \
	__used static void dma_wch_isr##ch(const struct device *dev)                               \
	{                                                                                          \
		if (ch <= DMA_WCH_MAX_CHAN_BASE) {                                                 \
			dma_wch_isr(dev, ch);                                                      \
		} else {                                                                           \
			dma_wch_isr_ext(dev, ch);                                                  \
		}                                                                                  \
	}

LISTIFY(DMA_WCH_MAX_CHAN, GENERATE_ISR, ())

#define IRQ_CONFIGURE(n, idx)                                                                      \
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(idx, n, irq), DT_INST_IRQ_BY_IDX(idx, n, priority),         \
		    dma_wch_isr##n, DEVICE_DT_INST_GET(idx), 0);                                   \
	irq_enable(DT_INST_IRQ_BY_IDX(idx, n, irq));

#define CONFIGURE_ALL_IRQS(idx, n) LISTIFY(n, IRQ_CONFIGURE, (), idx)

#define WCH_DMA_INIT(idx)                                                                          \
	static void dma_wch##idx##_irq_config(const struct device *dev)                            \
	{                                                                                          \
		CONFIGURE_ALL_IRQS(idx, DT_NUM_IRQS(DT_DRV_INST(idx)));                            \
	}                                                                                          \
	static const struct dma_wch_config dma_wch##idx##_config = {                               \
		.regs = (struct dma_wch_regs *)DT_INST_REG_ADDR(idx),                              \
		.num_channels = DT_INST_PROP(idx, dma_channels),                                   \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(idx)),                              \
		.clock_id = DT_INST_CLOCKS_CELL(idx, id),                                          \
		.irq_config_func = dma_wch##idx##_irq_config,                                      \
	};                                                                                         \
	static struct dma_wch_channel dma_wch##idx##_channels[DT_INST_PROP(idx, dma_channels)];    \
	ATOMIC_DEFINE(dma_wch_atomic##idx, DT_INST_PROP(idx, dma_channels));                       \
	static struct dma_wch_data dma_wch##idx##_data = {                                         \
		.ctx =                                                                             \
			{                                                                          \
				.magic = DMA_MAGIC,                                                \
				.atomic = dma_wch_atomic##idx,                                     \
				.dma_channels = DT_INST_PROP(idx, dma_channels),                   \
			},                                                                         \
		.channels = dma_wch##idx##_channels,                                               \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, &dma_wch_init, NULL, &dma_wch##idx##_data,                      \
			      &dma_wch##idx##_config, PRE_KERNEL_1, CONFIG_DMA_INIT_PRIORITY,      \
			      &dma_wch_driver_api);

DT_INST_FOREACH_STATUS_OKAY(WCH_DMA_INIT)
