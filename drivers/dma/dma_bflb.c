/*
 * Copyright (c) 2025 MASSDRIVER EI <massdriver.space>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bflb_dma

#include <zephyr/device.h>
#include <soc.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/cache.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dma_bflb, CONFIG_DMA_LOG_LEVEL);

#include <soc.h>
#include <bflb_soc.h>
#include <glb_reg.h>
#include <common_defines.h>
#include <bouffalolab/common/dma_reg.h>

#ifdef CONFIG_SOC_SERIES_BL61X
#define BFLB_DMA_CLOCK_ADDR (GLB_BASE + GLB_DMA_CFG0_OFFSET)
#else
#define BFLB_DMA_CLOCK_ADDR (GLB_BASE + GLB_CLK_CFG2_OFFSET)
#endif

#define BFLB_DMA_CH_OFFSET(n) ((n + 1) * 0x100)

#define BFLB_DMA_CH_NB DT_INST_PROP(0, dma_channels)

#define BFLB_DMA_WIDTH_BYTE  0
#define BFLB_DMA_WIDTH_2BYTE 1
#define BFLB_DMA_WIDTH_WORD  2
#define BFLB_DMA_WIDTH_2WORD 3

#define BFLB_DMA_BURST_1  0
#define BFLB_DMA_BURST_4  1
#define BFLB_DMA_BURST_8  2
#define BFLB_DMA_BURST_16 3

#define BFLB_DMA_FLOW_M_M  0
#define BFLB_DMA_FLOW_M_P  1
#define BFLB_DMA_FLOW_P_M  2
#define BFLB_DMA_FLOW_P_P  3
#define BFLB_DMA_FLOW_SOFT 0
#define BFLB_DMA_FLOW_PERI 4

struct dma_bflb_channel {
	dma_callback_t cb;
	void *user_data;
};

struct dma_bflb_data {
	struct dma_bflb_channel channels[BFLB_DMA_CH_NB];
};

struct dma_bflb_config {
	uint32_t base_reg;
};

static size_t dma_bflb_get_transfer_size(const struct device *dev, uint32_t channel)
{
	const struct dma_bflb_config *cfg = dev->config;
	uint32_t control;
	size_t size = 1;
	uint32_t width;

	control = sys_read32(cfg->base_reg + DMA_CxCONTROL_OFFSET + BFLB_DMA_CH_OFFSET(channel));

	width = (control & DMA_SWIDTH_MASK) >> DMA_SWIDTH_SHIFT;
	/* Using SOURCE data width */
	switch (width) {
	case BFLB_DMA_WIDTH_BYTE:
		break;
	case BFLB_DMA_WIDTH_2BYTE:
		size *= 2U;
		break;
	case BFLB_DMA_WIDTH_WORD:
		size *= 4U;
		break;
	case BFLB_DMA_WIDTH_2WORD:
		size *= 8U;
		break;
	default:
		return 0;
	}

	return size;
}

static void dma_bflb_isr(const struct device *dev)
{
	const struct dma_bflb_config *cfg = dev->config;
	struct dma_bflb_data *data = dev->data;
	uint32_t status, error;

	status = sys_read32(cfg->base_reg + DMA_INTTCSTATUS_OFFSET);
	error = sys_read32(cfg->base_reg + DMA_INTERRORSTATUS_OFFSET);

	for (uint8_t i = 0; i < BFLB_DMA_CH_NB; i++) {
		if (data->channels[i].cb) {
			if (error & (1 << i)) {
				data->channels[i].cb(dev, data->channels[i].user_data, i, -1);
			} else if (status & (1 << i)) {
				data->channels[i].cb(dev, data->channels[i].user_data, i, 0);
			}
		}
	}

	sys_write32(error, cfg->base_reg + DMA_INTERRCLR_OFFSET);
	sys_write32(status, cfg->base_reg + DMA_INTTCCLEAR_OFFSET);
}

static int dma_bflb_configure(const struct device *dev, uint32_t channel,
			   struct dma_config *config)
{
	struct dma_bflb_data *data = dev->data;
	const struct dma_bflb_config *cfg = dev->config;
	struct dma_block_config *block = config->head_block;
	struct dma_bflb_channel *channel_control;
	uint32_t ch_config;
	uint32_t control = 0;
	uint16_t size;

	if (channel >= BFLB_DMA_CH_NB) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	if (config->block_count > 1) {
		LOG_ERR("Chained transfers not supported");
		/* TODO: add support for LLI chained transfers. */
		return -ENOTSUP;
	}

	if (block->source_addr_adj == 1 || block->dest_addr_adj == 1) {
		LOG_ERR("Decrement not supported");
		return -EINVAL;
	}

	ch_config = sys_read32(cfg->base_reg
				+ DMA_CxCONFIG_OFFSET + BFLB_DMA_CH_OFFSET(channel));
	ch_config &= ~DMA_LLICOUNTER_MASK;

	ch_config &= ~DMA_FLOWCNTRL_MASK;
	if (config->channel_direction == MEMORY_TO_MEMORY) {
		ch_config |= BFLB_DMA_FLOW_M_M << DMA_FLOWCNTRL_SHIFT;
	} else if (config->channel_direction == PERIPHERAL_TO_MEMORY) {
		ch_config |= BFLB_DMA_FLOW_P_M << DMA_FLOWCNTRL_SHIFT;
	} else if (config->channel_direction == MEMORY_TO_PERIPHERAL) {
		ch_config |= BFLB_DMA_FLOW_M_P << DMA_FLOWCNTRL_SHIFT;
	} else if (config->channel_direction == PERIPHERAL_TO_PERIPHERAL) {
		ch_config |= BFLB_DMA_FLOW_P_P << DMA_FLOWCNTRL_SHIFT;
	} else {
		LOG_ERR("Direction error. %d", config->channel_direction);
		return -EINVAL;
	}

	/* For memory we write here */
	sys_write32(block->source_address, cfg->base_reg + DMA_CxSRCADDR_OFFSET
			+ BFLB_DMA_CH_OFFSET(channel));
	sys_write32(block->dest_address, cfg->base_reg + DMA_CxDSTADDR_OFFSET
			+ BFLB_DMA_CH_OFFSET(channel));

	/* For peripherals we treat the address as the peripheral ID */
	ch_config &= ~DMA_SRCPERIPHERAL_MASK;
	ch_config &= ~DMA_DSTPERIPHERAL_MASK;
	ch_config |= (block->source_address << DMA_SRCPERIPHERAL_SHIFT)
			& DMA_SRCPERIPHERAL_MASK;
	ch_config |= (block->dest_address << DMA_DSTPERIPHERAL_SHIFT)
			& DMA_DSTPERIPHERAL_MASK;

	if (!block->source_addr_adj) {
		control |= DMA_SI;
	}
	if (!block->dest_addr_adj) {
		control |= DMA_DI;
	}

	if (config->source_data_size == 1) {
		control |= BFLB_DMA_WIDTH_BYTE << DMA_SWIDTH_SHIFT;
	} else if (config->source_data_size == 2) {
		control |= BFLB_DMA_WIDTH_2BYTE << DMA_SWIDTH_SHIFT;
	} else if (config->source_data_size == 4) {
		control |= BFLB_DMA_WIDTH_WORD << DMA_SWIDTH_SHIFT;
	} else if (config->source_data_size == 8) {
		control |= BFLB_DMA_WIDTH_2WORD << DMA_SWIDTH_SHIFT;
	} else {
		LOG_ERR("Invalid source data size");
		return -EINVAL;
	}

	if (config->dest_data_size == 1) {
		control |= BFLB_DMA_WIDTH_BYTE << DMA_DWIDTH_SHIFT;
	} else if (config->dest_data_size == 2) {
		control |= BFLB_DMA_WIDTH_2BYTE << DMA_DWIDTH_SHIFT;
	} else if (config->dest_data_size == 4) {
		control |= BFLB_DMA_WIDTH_WORD << DMA_DWIDTH_SHIFT;
	} else if (config->dest_data_size == 8) {
		control |= BFLB_DMA_WIDTH_2WORD << DMA_DWIDTH_SHIFT;
	} else {
		LOG_ERR("Invalid destination data size");
		return -EINVAL;
	}

	if (config->source_burst_length == 1) {
		control |= BFLB_DMA_BURST_1 << DMA_SBSIZE_SHIFT;
	} else if (config->source_burst_length == 4) {
		control |= BFLB_DMA_BURST_4 << DMA_SBSIZE_SHIFT;
	} else if (config->source_burst_length == 8) {
		control |= BFLB_DMA_BURST_8 << DMA_SBSIZE_SHIFT;
	} else if (config->source_burst_length == 16) {
		control |= BFLB_DMA_BURST_16 << DMA_SBSIZE_SHIFT;
	} else {
		LOG_ERR("Invalid source burst size");
		return -EINVAL;
	}

	if (config->dest_burst_length == 1) {
		control |= BFLB_DMA_BURST_1 << DMA_DBSIZE_SHIFT;
	} else if (config->dest_burst_length == 4) {
		control |= BFLB_DMA_BURST_4 << DMA_DBSIZE_SHIFT;
	} else if (config->dest_burst_length == 8) {
		control |= BFLB_DMA_BURST_8 << DMA_DBSIZE_SHIFT;
	} else if (config->dest_burst_length == 16) {
		control |= BFLB_DMA_BURST_16 << DMA_DBSIZE_SHIFT;
	} else {
		LOG_ERR("Invalid destination burst size");
		return -EINVAL;
	}

	size = block->block_size / config->dest_data_size;
	control |= (size << DMA_TRANSFERSIZE_SHIFT) & DMA_TRANSFERSIZE_MASK;

	/* Clear interrupts */
	sys_write32(1U << channel, cfg->base_reg + DMA_INTERRCLR_OFFSET);
	sys_write32(1U << channel, cfg->base_reg + DMA_INTTCCLEAR_OFFSET);

	/* Unmask interrupts */
	ch_config &= ~(DMA_ITC | DMA_IE);

	sys_write32(control, cfg->base_reg
			+ DMA_CxCONTROL_OFFSET + BFLB_DMA_CH_OFFSET(channel));
	sys_write32(ch_config, cfg->base_reg
			+ DMA_CxCONFIG_OFFSET + BFLB_DMA_CH_OFFSET(channel));

	channel_control = &data->channels[channel];
	channel_control->cb = config->dma_callback;
	channel_control->user_data = config->user_data;

	LOG_DBG("Configured channel %d for %08X to %08X (%u)",
		channel,
		block->source_address,
		block->dest_address,
		block->block_size);

	return 0;
}

static int dma_bflb_start(const struct device *dev, uint32_t channel)
{
	const struct dma_bflb_config *cfg = dev->config;
	uint32_t config;

#ifdef CONFIG_SOC_SERIES_BL61X
	/* on BL61x, we must invalidate the output address to update the memory data */
	uint32_t control = sys_read32(
		cfg->base_reg + DMA_CxCONTROL_OFFSET + BFLB_DMA_CH_OFFSET(channel));
	size_t pending_length = (control & DMA_TRANSFERSIZE_MASK) >> DMA_TRANSFERSIZE_SHIFT;
	uintptr_t addr = sys_read32(
		cfg->base_reg + DMA_CxDSTADDR_OFFSET + BFLB_DMA_CH_OFFSET(channel));

	pending_length *= dma_bflb_get_transfer_size(dev, channel);

	sys_cache_data_flush_and_invd_range((void *)addr, pending_length);

	addr = sys_read32(
		cfg->base_reg + DMA_CxSRCADDR_OFFSET + BFLB_DMA_CH_OFFSET(channel));

	sys_cache_data_flush_and_invd_range((void *)addr, pending_length);
#endif

	config = sys_read32(cfg->base_reg + DMA_CxCONFIG_OFFSET + BFLB_DMA_CH_OFFSET(channel));
	config |= DMA_E;
	sys_write32(config, cfg->base_reg + DMA_CxCONFIG_OFFSET + BFLB_DMA_CH_OFFSET(channel));

	return 0;
}

static int dma_bflb_stop(const struct device *dev, uint32_t channel)
{
	const struct dma_bflb_config *cfg = dev->config;
	uint32_t config;

	config = sys_read32(cfg->base_reg + DMA_CxCONFIG_OFFSET + BFLB_DMA_CH_OFFSET(channel));
	config &= ~DMA_E;
	sys_write32(config, cfg->base_reg + DMA_CxCONFIG_OFFSET + BFLB_DMA_CH_OFFSET(channel));

	return 0;
}

static int dma_bflb_reload(const struct device *dev, uint32_t channel,
			   uint32_t src, uint32_t dst, size_t size)
{
	const struct dma_bflb_config *cfg = dev->config;
	uint32_t control;
	size_t sizediv;

	control = sys_read32(cfg->base_reg + DMA_CxCONTROL_OFFSET + BFLB_DMA_CH_OFFSET(channel));

	sizediv = dma_bflb_get_transfer_size(dev, channel);
	if (!sizediv) {
		return -EINVAL;
	}

	sys_write32(src, cfg->base_reg + DMA_CxSRCADDR_OFFSET + BFLB_DMA_CH_OFFSET(channel));
	sys_write32(dst, cfg->base_reg + DMA_CxDSTADDR_OFFSET + BFLB_DMA_CH_OFFSET(channel));

	size /= sizediv;
	control &= ~DMA_TRANSFERSIZE_MASK;
	control |= (size << DMA_TRANSFERSIZE_SHIFT) & DMA_TRANSFERSIZE_MASK;
	sys_write32(control, cfg->base_reg + DMA_CxCONTROL_OFFSET + BFLB_DMA_CH_OFFSET(channel));

	LOG_DBG("Reloaded channel %d for %08X to %08X (%u)",
		channel, src, dst, size);

	return 0;
}

static int dma_bflb_get_status(const struct device *dev, uint32_t channel,
			       struct dma_status *stat)
{
	const struct dma_bflb_config *cfg = dev->config;
	uint32_t config, control, direction;

	if (channel >= BFLB_DMA_CH_NB || stat == NULL) {
		return -EINVAL;
	}

	config = sys_read32(cfg->base_reg + DMA_CxCONFIG_OFFSET + BFLB_DMA_CH_OFFSET(channel));
	control = sys_read32(cfg->base_reg + DMA_CxCONTROL_OFFSET + BFLB_DMA_CH_OFFSET(channel));

	if (config & DMA_E) {
		stat->busy = true;
	} else {
		stat->busy = false;
	}

	stat->pending_length = (control & DMA_TRANSFERSIZE_MASK) >> DMA_TRANSFERSIZE_SHIFT;
	stat->pending_length *= dma_bflb_get_transfer_size(dev, channel);
	if (!stat->pending_length) {
		return -EINVAL;
	}

	direction = ((config & DMA_FLOWCNTRL_MASK) >> DMA_FLOWCNTRL_SHIFT) & BFLB_DMA_FLOW_P_P;
	switch (direction) {
	case BFLB_DMA_FLOW_M_M:
		stat->dir = MEMORY_TO_MEMORY;
		break;
	case BFLB_DMA_FLOW_M_P:
		stat->dir = MEMORY_TO_PERIPHERAL;
		break;
	case BFLB_DMA_FLOW_P_M:
		stat->dir = PERIPHERAL_TO_MEMORY;
		break;
	case BFLB_DMA_FLOW_P_P:
		stat->dir = PERIPHERAL_TO_PERIPHERAL;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int dma_bflb_init(const struct device *dev)
{
	const struct dma_bflb_config *cfg = dev->config;
	uint32_t tmp;

	/* Ensure DMA clocks are enabled*/
	tmp = sys_read32(BFLB_DMA_CLOCK_ADDR);
	tmp |= 0xFF << GLB_DMA_CLK_EN_POS;
	sys_write32(tmp, BFLB_DMA_CLOCK_ADDR);

	/* Enable DMA controller */
	tmp = sys_read32(cfg->base_reg + DMA_TOP_CONFIG_OFFSET);
	tmp |= DMA_E;
	sys_write32(tmp, cfg->base_reg + DMA_TOP_CONFIG_OFFSET);

	/* Ensure all channels are disabled and their interrupts masked */
	for (int i = 0; i < BFLB_DMA_CH_NB; i++) {
		tmp = sys_read32(cfg->base_reg + DMA_CxCONFIG_OFFSET + BFLB_DMA_CH_OFFSET(i));
		tmp &= ~DMA_E;
		tmp |= DMA_ITC | DMA_IE;
		sys_write32(tmp, cfg->base_reg + DMA_CxCONFIG_OFFSET + BFLB_DMA_CH_OFFSET(i));
		tmp = sys_read32(cfg->base_reg + DMA_CxCONTROL_OFFSET + BFLB_DMA_CH_OFFSET(i));
		tmp &= ~DMA_I;
		sys_write32(tmp, cfg->base_reg + DMA_CxCONTROL_OFFSET + BFLB_DMA_CH_OFFSET(i));
	}

	/* Ensure all interrupts are cleared */
	sys_write32(0xFF, cfg->base_reg + DMA_INTERRCLR_OFFSET);
	sys_write32(0xFF, cfg->base_reg + DMA_INTTCCLEAR_OFFSET);

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), dma_bflb_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));

	return 0;
}

static struct dma_bflb_data bflb_dma_data = {0};
static struct dma_bflb_config bflb_dma_config = {
	.base_reg = DT_INST_REG_ADDR(0),
};

static DEVICE_API(dma, dma_bflb_api) = {
	.config = dma_bflb_configure,
	.start = dma_bflb_start,
	.stop = dma_bflb_stop,
	.reload = dma_bflb_reload,
	.get_status = dma_bflb_get_status,
};

DEVICE_DT_INST_DEFINE(0, dma_bflb_init, NULL,
		      &bflb_dma_data, &bflb_dma_config, PRE_KERNEL_1,
		      CONFIG_DMA_INIT_PRIORITY, &dma_bflb_api);
