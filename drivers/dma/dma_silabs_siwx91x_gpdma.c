/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/sys/mem_blocks.h>
#include <zephyr/spinlock.h>
#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>
#include "rsi_gpdma.h"
#include "rsi_rom_gpdma.h"

#define DT_DRV_COMPAT                silabs_gpdma
#define GPDMA_DESC_MAX_TRANSFER_SIZE 4096
#define GPDMA_MAX_CHANNEL_FIFO_SIZE  64
#define GPDMA_MAX_PRIORITY           3
#define GPDMA_MIN_PRIORITY           0

LOG_MODULE_REGISTER(si91x_gpdma, CONFIG_DMA_LOG_LEVEL);

enum gpdma_xfer_dir {
	SIWX91X_TRANSFER_MEM_TO_MEM = 0,
	SIWX91X_TRANSFER_MEM_TO_PER = 1,
	SIWX91X_TRANSFER_PER_TO_MEM = 2,
	SIWX91X_TRANSFER_DIR_INVALID = 4,
};

enum gpdma_data_width {
	SIWX91X_DATA_WIDTH_8 = 0,
	SIWX91X_DATA_WIDTH_16 = 1,
	SIWX91X_DATA_WIDTH_32 = 2,
	SIWX91X_DATA_WIDTH_INVALID = 3,
};

struct siwx91x_gpdma_channel_info {
	dma_callback_t cb;
	void *cb_data;
	RSI_GPDMA_DESC_T *desc;
	enum gpdma_xfer_dir xfer_direction;
};

struct siwx91x_gpdma_config {
	GPDMA_G_Type *reg;         /* GPDMA base address */
	GPDMA_C_Type *channel_reg; /* GPDMA channel registers address */
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	void (*irq_configure)(void);
};

struct siwx19x_gpdma_data {
	struct dma_context dma_ctx;
	GPDMA_DATACONTEXT_T hal_ctx;
	struct sys_mem_blocks *desc_pool;
	struct k_spinlock desc_pool_lock;
	struct siwx91x_gpdma_channel_info *chan_info;
	uint8_t reload_compatible;
};

static bool siwx91x_gpdma_is_priority_valid(uint32_t channel_priority)
{
	return (channel_priority >= GPDMA_MIN_PRIORITY && channel_priority <= GPDMA_MAX_PRIORITY);
}

static enum gpdma_xfer_dir siwx91x_gpdma_xfer_dir(uint32_t dir)
{
	switch (dir) {
	case MEMORY_TO_MEMORY:
		return SIWX91X_TRANSFER_MEM_TO_MEM;
	case MEMORY_TO_PERIPHERAL:
		return SIWX91X_TRANSFER_MEM_TO_PER;
	case PERIPHERAL_TO_MEMORY:
		return SIWX91X_TRANSFER_PER_TO_MEM;
	default:
		return SIWX91X_TRANSFER_DIR_INVALID;
	}
}

static enum gpdma_data_width siwx91x_gpdma_data_size(uint32_t data_size)
{
	switch (data_size) {
	case 1:
		return SIWX91X_DATA_WIDTH_8;
	case 2:
		return SIWX91X_DATA_WIDTH_16;
	case 4:
		return SIWX91X_DATA_WIDTH_32;
	default:
		return SIWX91X_DATA_WIDTH_INVALID;
	}
}

static int siwx91x_gpdma_ahb_blen(uint8_t blen)
{
	if (blen == 1) {
		return AHBBURST_SIZE_1;
	} else if (blen <= 4) {
		return AHBBURST_SIZE_4;
	} else if (blen <= 8) {
		return AHBBURST_SIZE_8;
	} else if (blen <= 16) {
		return AHBBURST_SIZE_16;
	} else if (blen <= 20) {
		return AHBBURST_SIZE_20;
	} else if (blen <= 24) {
		return AHBBURST_SIZE_24;
	} else if (blen <= 28) {
		return AHBBURST_SIZE_28;
	} else if (blen <= 32) {
		return AHBBURST_SIZE_32;
	} else {
		return -EINVAL;
	}
}

static void siwx91x_gpdma_free_desc(sys_mem_blocks_t *mem_block, RSI_GPDMA_DESC_T *block)
{
	RSI_GPDMA_DESC_T *cur_desc = block;
	uint32_t *next_desc = NULL;

	do {
		next_desc = cur_desc->pNextLink;
		sys_mem_blocks_free(mem_block, 1, (void **)&cur_desc);
		cur_desc = (RSI_GPDMA_DESC_T *)next_desc;
	} while (next_desc != NULL);
}

static int siwx91x_gpdma_desc_config(struct siwx19x_gpdma_data *data,
				     const struct dma_config *config,
				     const RSI_GPDMA_DESC_T *xfer_cfg, uint32_t channel)
{
	uint16_t max_xfer_size = GPDMA_DESC_MAX_TRANSFER_SIZE - config->source_data_size;
	const struct dma_block_config *block_addr = config->head_block;
	RSI_GPDMA_DESC_T *cur_desc = NULL;
	RSI_GPDMA_DESC_T *prev_desc = NULL;
	k_spinlock_key_t key;
	int ret;

	for (int i = 0; i < config->block_count; i++) {
		if (block_addr->block_size > max_xfer_size) {
			LOG_ERR("Maximum xfer size should be <= %d\n", max_xfer_size);
			goto free_desc;
		}

		key = k_spin_lock(&data->desc_pool_lock);
		ret = sys_mem_blocks_alloc(data->desc_pool, 1, (void **)&cur_desc);
		k_spin_unlock(&data->desc_pool_lock, key);
		if (ret) {
			goto free_desc;
		}

		if (prev_desc == NULL) {
			data->chan_info[channel].desc = cur_desc;
		}

		memset(cur_desc, 0, 32);

		ret = RSI_GPDMA_BuildDescriptors(&data->hal_ctx, (RSI_GPDMA_DESC_T *)xfer_cfg,
						 cur_desc, prev_desc);
		if (ret) {
			goto free_desc;
		}

		cur_desc->src = (void *)block_addr->source_address;
		cur_desc->dest = (void *)block_addr->dest_address;
		cur_desc->chnlCtrlConfig.transSize = block_addr->block_size;

		if (block_addr->dest_addr_adj == DMA_ADDR_ADJ_NO_CHANGE) {
			cur_desc->chnlCtrlConfig.dstFifoMode = 1;
		}

		if (block_addr->source_addr_adj == DMA_ADDR_ADJ_NO_CHANGE) {
			cur_desc->chnlCtrlConfig.srcFifoMode = 1;
		}

		prev_desc = cur_desc;
		block_addr = block_addr->next_block;
	}

	if (block_addr != NULL) {
		/* next_block address for last block must be null */
		goto free_desc;
	}

	ret = RSI_GPDMA_SetupChannelTransfer(&data->hal_ctx, channel,
					     data->chan_info[channel].desc);
	if (ret) {
		goto free_desc;
	}

	return 0;

free_desc:
	key = k_spin_lock(&data->desc_pool_lock);
	siwx91x_gpdma_free_desc(data->desc_pool, data->chan_info[channel].desc);
	k_spin_unlock(&data->desc_pool_lock, key);
	return -EINVAL;
}

static int siwx91x_gpdma_xfer_configure(const struct device *dev, const struct dma_config *config,
					uint32_t channel)
{
	struct siwx19x_gpdma_data *data = dev->data;
	RSI_GPDMA_DESC_T xfer_cfg = {};
	int ret;

	xfer_cfg.chnlCtrlConfig.transType = siwx91x_gpdma_xfer_dir(config->channel_direction);
	if (xfer_cfg.chnlCtrlConfig.transType == SIWX91X_TRANSFER_DIR_INVALID) {
		return -EINVAL;
	}

	data->chan_info[channel].xfer_direction = config->channel_direction;

	if (config->dest_data_size != config->source_data_size) {
		LOG_ERR("Data size mismatch\n");
		return -EINVAL;
	}

	if (config->dest_burst_length != config->source_burst_length) {
		LOG_ERR("Burst length mismatch\n");
		return -EINVAL;
	}

	if (config->source_data_size * config->source_burst_length >= GPDMA_MAX_CHANNEL_FIFO_SIZE) {
		LOG_ERR("FIFO overflow detected: data_size × burst_length = %d >= %d (maximum "
			"allowed)\n",
			config->source_data_size * config->source_burst_length,
			GPDMA_MAX_CHANNEL_FIFO_SIZE);
		return -EINVAL;
	}

	xfer_cfg.chnlCtrlConfig.srcDataWidth = siwx91x_gpdma_data_size(config->source_data_size);
	if (xfer_cfg.chnlCtrlConfig.destDataWidth == SIWX91X_DATA_WIDTH_INVALID) {
		return -EINVAL;
	}

	xfer_cfg.chnlCtrlConfig.destDataWidth = xfer_cfg.chnlCtrlConfig.srcDataWidth;

	if (config->block_count == 1) {
		xfer_cfg.chnlCtrlConfig.linkListOn = 0;
		data->reload_compatible = 1;
	} else {
		xfer_cfg.chnlCtrlConfig.linkListOn = 1;
		data->reload_compatible = 0;
	}
	xfer_cfg.chnlCtrlConfig.linkInterrupt = config->complete_callback_en;

	if (xfer_cfg.chnlCtrlConfig.transType == SIWX91X_TRANSFER_MEM_TO_PER) {
		xfer_cfg.miscChnlCtrlConfig.destChannelId = config->dma_slot;
	} else if (xfer_cfg.chnlCtrlConfig.transType == SIWX91X_TRANSFER_PER_TO_MEM) {
		xfer_cfg.miscChnlCtrlConfig.srcChannelId = config->dma_slot;
	}

	xfer_cfg.miscChnlCtrlConfig.ahbBurstSize =
		siwx91x_gpdma_ahb_blen(config->source_burst_length);
	xfer_cfg.miscChnlCtrlConfig.destDataBurst = config->dest_burst_length;
	xfer_cfg.miscChnlCtrlConfig.srcDataBurst = config->source_burst_length;

	ret = siwx91x_gpdma_desc_config(data, config, &xfer_cfg, channel);
	if (ret) {
		return ret;
	}

	return 0;
}

static int siwx91x_gpdma_configure(const struct device *dev, uint32_t channel,
				   struct dma_config *config)
{
	const struct siwx91x_gpdma_config *cfg = dev->config;
	struct siwx19x_gpdma_data *data = dev->data;
	RSI_GPDMA_CHA_CFG_T gpdma_channel_cfg = {
		.descFetchDoneIntr = config->complete_callback_en,
		.hrespErr = 1,
		.gpdmacErr = 1,
		.xferDoneIntr = 1,
		.dmaCh = channel,
	};
	int ret;

	if (channel >= data->dma_ctx.dma_channels) {
		return -EINVAL;
	}

	if (config->cyclic) {
		return -ENOTSUP;
	}

	if (sys_test_bit((mem_addr_t)&cfg->reg->GLOBAL.DMA_CHNL_ENABLE_REG, channel)) {
		/* Transfer in progress */
		return -EIO;
	}

	if (!siwx91x_gpdma_is_priority_valid(config->channel_priority)) {
		LOG_ERR("Invalid priority values: (valid range: 0-3)\n");
		return -EINVAL;
	}
	gpdma_channel_cfg.channelPrio = config->channel_priority;

	if (RSI_GPDMA_SetupChannel(&data->hal_ctx, &gpdma_channel_cfg)) {
		return -EIO;
	}

	ret = siwx91x_gpdma_xfer_configure(dev, config, channel);
	if (ret) {
		return -EINVAL;
	}

	cfg->channel_reg->CHANNEL_CONFIG[channel].FIFO_CONFIG_REGS = 0;
	/* Allocate 8 rows of FIFO for each channel (64 bytes) */
	cfg->channel_reg->CHANNEL_CONFIG[channel].FIFO_CONFIG_REGS_b.FIFO_SIZE = 7;
	cfg->channel_reg->CHANNEL_CONFIG[channel].FIFO_CONFIG_REGS_b.FIFO_STRT_ADDR = (channel * 8);

	data->chan_info[channel].cb = config->dma_callback;
	data->chan_info[channel].cb_data = config->user_data;

	return 0;
}

static int siwx91x_gpdma_reload(const struct device *dev, uint32_t channel, uint32_t src,
				uint32_t dst, size_t size)
{
	const struct siwx91x_gpdma_config *cfg = dev->config;
	struct siwx19x_gpdma_data *data = dev->data;
	int data_size = siwx91x_gpdma_data_size(
		cfg->channel_reg->CHANNEL_CONFIG[channel].CHANNEL_CTRL_REG_CHNL_b.SRC_DATA_WIDTH);

	if (channel >= data->dma_ctx.dma_channels) {
		return -EINVAL;
	}

	if (!data->reload_compatible) {
		return -ENOTSUP;
	}

	if (sys_test_bit((mem_addr_t)&cfg->reg->GLOBAL.DMA_CHNL_ENABLE_REG, channel)) {
		/* Transfer in progress */
		return -EIO;
	}

	if (data_size < 0) {
		return -EINVAL;
	}

	if (size > (GPDMA_DESC_MAX_TRANSFER_SIZE - data_size)) {
		LOG_ERR("Maximum xfer size should be <= %d\n",
			GPDMA_DESC_MAX_TRANSFER_SIZE - data_size);
		return -EINVAL;
	}

	cfg->channel_reg->CHANNEL_CONFIG[channel].SRC_ADDR_REG_CHNL = src;
	cfg->channel_reg->CHANNEL_CONFIG[channel].DEST_ADDR_REG_CHNL = dst;
	cfg->channel_reg->CHANNEL_CONFIG[channel].CHANNEL_CTRL_REG_CHNL_b.DMA_BLK_SIZE = size;

	return 0;
}

static int siwx91x_gpdma_start(const struct device *dev, uint32_t channel)
{
	struct siwx19x_gpdma_data *data = dev->data;

	if (channel >= data->dma_ctx.dma_channels) {
		return -EINVAL;
	}

	if (RSI_GPDMA_DMAChannelTrigger(&data->hal_ctx, channel)) {
		return -EINVAL;
	}

	return 0;
}

static int siwx91x_gpdma_stop(const struct device *dev, uint32_t channel)
{
	struct siwx19x_gpdma_data *data = dev->data;
	k_spinlock_key_t key;

	if (channel >= data->dma_ctx.dma_channels) {
		return -EINVAL;
	}

	if (RSI_GPDMA_AbortChannel(&data->hal_ctx, channel)) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->desc_pool_lock);
	siwx91x_gpdma_free_desc(data->desc_pool, data->chan_info[channel].desc);
	k_spin_unlock(&data->desc_pool_lock, key);

	return 0;
}

static int siwx91x_gpdma_get_status(const struct device *dev, uint32_t channel,
				    struct dma_status *stat)
{
	const struct siwx91x_gpdma_config *cfg = dev->config;
	struct siwx19x_gpdma_data *data = dev->data;

	if (channel >= data->dma_ctx.dma_channels) {
		return -EINVAL;
	}

	if (!atomic_test_bit(data->dma_ctx.atomic, channel)) {
		return -EINVAL;
	}

	stat->busy = sys_test_bit((mem_addr_t)&cfg->reg->GLOBAL.DMA_CHNL_ENABLE_REG, channel);
	stat->dir = data->chan_info[channel].xfer_direction;

	return 0;
}

bool siwx91x_gpdma_chan_filter(const struct device *dev, int channel, void *filter_param)
{
	ARG_UNUSED(dev);

	if (!filter_param) {
		return false;
	}

	if (*(int *)filter_param == channel) {
		return true;
	} else {
		return false;
	}
}

static int siwx91x_gpdma_init(const struct device *dev)
{
	const struct siwx91x_gpdma_config *cfg = dev->config;
	struct siwx19x_gpdma_data *data = dev->data;
	RSI_GPDMA_INIT_T gpdma_init = {
		.pUserData = NULL,
		.baseG = (uint32_t)cfg->reg,
		.baseC = (uint32_t)cfg->channel_reg,
		.sramBase = (uint32_t)data->desc_pool->buffer,
	};
	RSI_GPDMA_HANDLE_T gpdma_handle;
	int ret;

	ret = clock_control_on(cfg->clock_dev, cfg->clock_subsys);
	if (ret) {
		return ret;
	}

	gpdma_handle = RSI_GPDMA_Init(&data->hal_ctx, &gpdma_init);
	if (gpdma_handle != &data->hal_ctx) {
		return -EIO;
	}

	cfg->irq_configure();

	return 0;
}

static void siwx91x_gpdma_isr(const struct device *dev)
{
	const struct siwx91x_gpdma_config *cfg = dev->config;
	struct siwx19x_gpdma_data *data = dev->data;
	uint32_t channel = find_lsb_set(cfg->reg->GLOBAL.INTERRUPT_REG) - 1;
	uint32_t channel_int_status = cfg->reg->GLOBAL.INTERRUPT_STAT_REG;
	uint32_t channel_shift = channel * 4;
	uint32_t abort_mask = (BIT(0) | BIT(3)) << channel_shift;
	uint32_t desc_fetch_mask = BIT(1) << channel_shift;
	uint32_t done_mask = BIT(2) << channel_shift;
	k_spinlock_key_t key;

	if (channel_int_status & abort_mask) {
		RSI_GPDMA_AbortChannel(&data->hal_ctx, channel);
		cfg->reg->GLOBAL.INTERRUPT_STAT_REG = abort_mask;
	}

	if (channel_int_status & desc_fetch_mask) {
		cfg->reg->GLOBAL.INTERRUPT_STAT_REG = desc_fetch_mask;
		if (data->chan_info[channel].cb) {
			data->chan_info[channel].cb(dev, data->chan_info[channel].cb_data, channel,
						    0);
		}
	}

	if (channel_int_status & done_mask) {
		key = k_spin_lock(&data->desc_pool_lock);
		siwx91x_gpdma_free_desc(data->desc_pool, data->chan_info[channel].desc);
		k_spin_unlock(&data->desc_pool_lock, key);
		data->chan_info[channel].desc = NULL;
		cfg->reg->GLOBAL.INTERRUPT_STAT_REG = done_mask;
		if (data->chan_info[channel].cb) {
			data->chan_info[channel].cb(dev, data->chan_info[channel].cb_data, channel,
						    0);
		}
	}
}

static DEVICE_API(dma, siwx91x_gpdma_api) = {
	.config = siwx91x_gpdma_configure,
	.reload = siwx91x_gpdma_reload,
	.start = siwx91x_gpdma_start,
	.stop = siwx91x_gpdma_stop,
	.get_status = siwx91x_gpdma_get_status,
	.chan_filter = siwx91x_gpdma_chan_filter,
};

#define SIWX91X_GPDMA_INIT(inst)                                                                   \
	static ATOMIC_DEFINE(siwx91x_gdma_atomic_##inst,                                           \
			     DT_INST_PROP(inst, silabs_dma_channel_count));                        \
	SYS_MEM_BLOCKS_DEFINE_STATIC(siwx91x_gpdma_desc_pool_##inst, 32,                           \
				     CONFIG_GPDMA_SILABS_SIWX91X_DESCRIPTOR_COUNT, 32);            \
	static struct siwx91x_gpdma_channel_info                                                   \
		siwx91x_gpdma_chan_info_##inst[DT_INST_PROP(inst, silabs_dma_channel_count)];      \
	static struct siwx19x_gpdma_data siwx91x_gpdma_data_##inst = {                             \
		.dma_ctx.magic = DMA_MAGIC,                                                        \
		.dma_ctx.dma_channels = DT_INST_PROP(inst, silabs_dma_channel_count),              \
		.dma_ctx.atomic = siwx91x_gdma_atomic_##inst,                                      \
		.desc_pool = &siwx91x_gpdma_desc_pool_##inst,                                      \
		.chan_info = siwx91x_gpdma_chan_info_##inst,                                       \
	};                                                                                         \
	static void siwx91x_gpdma_irq_configure_##inst(void)                                       \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ(inst, irq), DT_INST_IRQ(inst, priority),                   \
			    siwx91x_gpdma_isr, DEVICE_DT_INST_GET(inst), 0);                       \
		irq_enable(DT_INST_IRQ(inst, irq));                                                \
	}                                                                                          \
                                                                                                   \
	static const struct siwx91x_gpdma_config siwx91x_gpdma_cfg_##inst = {                      \
		.reg = (GPDMA_G_Type *)DT_INST_REG_ADDR(inst),                                     \
		.channel_reg = (GPDMA_C_Type *)DT_INST_PROP(inst, silabs_channel_reg_base),        \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                             \
		.clock_subsys = (clock_control_subsys_t)DT_INST_PHA(inst, clocks, clkid),          \
		.irq_configure = siwx91x_gpdma_irq_configure_##inst,                               \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, siwx91x_gpdma_init, NULL, &siwx91x_gpdma_data_##inst,          \
			      &siwx91x_gpdma_cfg_##inst, POST_KERNEL, CONFIG_DMA_INIT_PRIORITY,    \
			      &siwx91x_gpdma_api);

DT_INST_FOREACH_STATUS_OKAY(SIWX91X_GPDMA_INIT)
