/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT silabs_gpdma
#include <zephyr/irq.h>
#include <zephyr/sys/mem_blocks.h>
#include <zephyr/spinlock.h>
#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include "rsi_gpdma.h"
#include "rsi_rom_gpdma.h"

#define SIWX91X_GPDMA_MAX_XFER_COUNT      4096
#define SIWX91X_GPDMA_MAX_FIFO_SIZE         64
#define SIWX91X_GPDMA_MAX_PRIORITY           3
#define SIWX91X_GPDMA_MIN_PRIORITY           0

LOG_MODULE_REGISTER(siwx91x_gpdma, CONFIG_DMA_LOG_LEVEL);

enum siwx91x_gpdma_xfer_dir {
	SIWX91X_TRANSFER_MEM_TO_MEM = 0,
	SIWX91X_TRANSFER_MEM_TO_PER = 1,
	SIWX91X_TRANSFER_PER_TO_MEM = 2,
	SIWX91X_TRANSFER_DIR_INVALID = 4,
};

enum siwx91x_gpdma_data_width {
	SIWX91X_DATA_WIDTH_8 = 0,
	SIWX91X_DATA_WIDTH_16 = 1,
	SIWX91X_DATA_WIDTH_32 = 2,
	SIWX91X_DATA_WIDTH_INVALID = 3,
};

struct siwx91x_gpdma_channel_info {
	dma_callback_t cb;
	void *cb_data;
	RSI_GPDMA_DESC_T *desc;
	uint32_t xfer_direction;
	bool channel_active;
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
	if (channel_priority < SIWX91X_GPDMA_MIN_PRIORITY ||
	    channel_priority > SIWX91X_GPDMA_MAX_PRIORITY) {
		return false;
	}
	return true;
}

static enum siwx91x_gpdma_xfer_dir siwx91x_gpdma_xfer_dir(uint32_t dir)
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

static enum siwx91x_gpdma_data_width siwx91x_gpdma_data_size(uint32_t data_size)
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

static int siwx91x_gpdma_blen(uint8_t blen)
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

static void siwx91x_gpdma_free_desc(struct siwx19x_gpdma_data *data, RSI_GPDMA_DESC_T *block)
{
	RSI_GPDMA_DESC_T *cur_desc = block;
	RSI_GPDMA_DESC_T *next_desc;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->desc_pool_lock);
	while (cur_desc) {
		next_desc = (RSI_GPDMA_DESC_T *)cur_desc->pNextLink;
		sys_mem_blocks_free(data->desc_pool, 1, (void **)&cur_desc);
		cur_desc = next_desc;
	}
	k_spin_unlock(&data->desc_pool_lock, key);
}

static RSI_GPDMA_DESC_T *siwx91x_gpdma_alloc_desc(struct siwx19x_gpdma_data *data, int count)
{
	RSI_GPDMA_DESC_T *head = NULL;
	RSI_GPDMA_DESC_T *desc;
	k_spinlock_key_t key;
	int ret;

	for (int i = 0; i < count; i++) {
		key = k_spin_lock(&data->desc_pool_lock);
		ret = sys_mem_blocks_alloc(data->desc_pool, 1, (void **)&desc);
		k_spin_unlock(&data->desc_pool_lock, key);
		if (ret) {
			siwx91x_gpdma_free_desc(data, head);
			return NULL;
		}
		desc->pNextLink = (void *)head;
		head = desc;
	}

	return head;
}

static int siwx91x_gpdma_count_hw_desc(const struct dma_config *config, int operation_width)
{
	const struct dma_block_config *block = config->head_block;
	int total = 0;

	for (int i = 0; i < config->block_count; i++) {
		/* DMA_ADDR_ADJ_NO_CHANGE consumes 1 descriptor */
		if (block->dest_addr_adj == DMA_ADDR_ADJ_NO_CHANGE &&
		    config->channel_direction == PERIPHERAL_TO_MEMORY) {
			total += 1;
		} else {
			total += DIV_ROUND_UP(block->block_size,
					      SIWX91X_GPDMA_MAX_XFER_COUNT - operation_width);
		}
		block = block->next_block;
	}

	return total;
}

int siwx91x_gpdma_get_template_descr(struct siwx19x_gpdma_data *data,
				     const struct dma_config *config,
				     const struct dma_block_config *block,
				     RSI_GPDMA_DESC_T *tmpl)
{
	tmpl->chnlCtrlConfig.transType = siwx91x_gpdma_xfer_dir(config->channel_direction);
	tmpl->chnlCtrlConfig.srcDataWidth = siwx91x_gpdma_data_size(config->source_data_size);
	tmpl->chnlCtrlConfig.destDataWidth = siwx91x_gpdma_data_size(config->dest_data_size);
	tmpl->chnlCtrlConfig.linkListOn = !data->reload_compatible;
	tmpl->chnlCtrlConfig.linkInterrupt = config->complete_callback_en;
	tmpl->chnlCtrlConfig.dstFifoMode = !!(block->dest_addr_adj == DMA_ADDR_ADJ_NO_CHANGE);
	tmpl->chnlCtrlConfig.srcFifoMode = !!(block->source_addr_adj == DMA_ADDR_ADJ_NO_CHANGE);
	tmpl->miscChnlCtrlConfig.ahbBurstSize = siwx91x_gpdma_blen(config->source_burst_length);
	tmpl->miscChnlCtrlConfig.srcDataBurst = config->source_burst_length;
	tmpl->miscChnlCtrlConfig.destDataBurst = config->dest_burst_length;
	if (config->channel_direction == PERIPHERAL_TO_MEMORY) {
		tmpl->miscChnlCtrlConfig.srcChannelId = config->dma_slot;
	}
	if (config->channel_direction == MEMORY_TO_PERIPHERAL) {
		tmpl->miscChnlCtrlConfig.destChannelId = config->dma_slot;
		if (block->source_addr_adj == DMA_ADDR_ADJ_NO_CHANGE) {
			/* HACK: GPDMA does not support DMA_ADDR_ADJ_NO_CHANGE with a memory buffer.
			 * Fill the peripheral with 0x00 or 0xFF instead.
			 */
			tmpl->miscChnlCtrlConfig.memoryFillEn = 1;
			if (*(uint8_t *)block->source_address == 0xFF) {
				tmpl->miscChnlCtrlConfig.memoryOneFill = 1;
			} else if (*(uint8_t *)block->source_address != 0x00) {
				LOG_ERR("Only 0xFF and 0x00 are supported as input");
				return -EINVAL;
			}
		}
	}
	return 0;
}

static RSI_GPDMA_DESC_T *siwx91x_gpdma_block_config(struct siwx19x_gpdma_data *data,
						    const struct dma_config *config,
						    const struct dma_block_config *block)
{
	int operation_width = config->source_data_size * config->source_burst_length;
	uint32_t max_chunk = SIWX91X_GPDMA_MAX_XFER_COUNT - operation_width;
	uint32_t src_addr = block->source_address;
	uint32_t dst_addr = block->dest_address;
	uint32_t remaining = block->block_size;
	RSI_GPDMA_DESC_T tmpl;
	RSI_GPDMA_DESC_T *desc;
	RSI_GPDMA_DESC_T *head;

	if (siwx91x_gpdma_get_template_descr(data, config, block, &tmpl)) {
		return NULL;
	}

	/* HACK: GPDMA does not support DMA_ADDR_ADJ_NO_CHANGE with a memory buffer.
	 * This configuration is mainly used by SPI to ignore the received bytes.
	 * The hack below disables the data transfer (reduces the length to one byte).
	 * Fortunately, SPI only watches DMA Tx termination so the early termination of
	 * the DMA Rx won't hurt.
	 */
	if (block->dest_addr_adj == DMA_ADDR_ADJ_NO_CHANGE &&
	    config->channel_direction == PERIPHERAL_TO_MEMORY) {
		if (block->next_block &&
		    block->next_block->dest_addr_adj != DMA_ADDR_ADJ_NO_CHANGE) {
			/* It is not possible to receive a real buffer after the hack above */
			LOG_ERR("Buffer interleaving is not supported");
			return NULL;
		}
		remaining = 1;
	}

	head = siwx91x_gpdma_alloc_desc(data, DIV_ROUND_UP(remaining, max_chunk));
	if (!head) {
		return NULL;
	}

	desc = head;
	while (remaining > 0) {
		uint32_t chunk = MIN(remaining, max_chunk);
		RSI_GPDMA_DESC_T *next = (void *)desc->pNextLink;

		memcpy(desc, &tmpl, sizeof(tmpl));
		desc->chnlCtrlConfig.transSize = chunk;
		desc->src = (void *)src_addr;
		desc->dest = (void *)dst_addr;
		desc->pNextLink = (void *)next;

		remaining -= chunk;
		if (block->source_addr_adj == DMA_ADDR_ADJ_INCREMENT) {
			src_addr += chunk;
		}
		if (block->dest_addr_adj == DMA_ADDR_ADJ_INCREMENT) {
			dst_addr += chunk;
		}
		desc = next;
	}

	return head;
}

static int siwx91x_gpdma_desc_config(struct siwx19x_gpdma_data *data,
				     const struct dma_config *config,
				     struct siwx91x_gpdma_channel_info *chan_info)
{
	int operation_width = config->source_data_size * config->source_burst_length;
	const struct dma_block_config *block = config->head_block;
	RSI_GPDMA_DESC_T *prev = NULL;
	RSI_GPDMA_DESC_T *head;

	chan_info->desc = NULL;
	for (int i = 0; i < config->block_count; i++) {
		if (!IS_ALIGNED(block->source_address, config->source_burst_length) ||
		    !IS_ALIGNED(block->dest_address, config->dest_burst_length) ||
		    !IS_ALIGNED(block->block_size, operation_width)) {
			LOG_ERR("Buffer not aligned");
			goto fail;
		}

		head = siwx91x_gpdma_block_config(data, config, block);
		if (!head) {
			goto fail;
		}

		if (prev) {
			prev->pNextLink = (void *)head;
		} else {
			chan_info->desc = head;
		}
		prev = head;
		while (prev->pNextLink) {
			prev = (RSI_GPDMA_DESC_T *)prev->pNextLink;
		}
		block = block->next_block;
	}

	if (block) {
		goto fail;
	}

	return 0;

fail:
	siwx91x_gpdma_free_desc(data, chan_info->desc);
	chan_info->desc = NULL;
	return -EINVAL;
}

static int siwx91x_gpdma_xfer_configure(const struct device *dev, const struct dma_config *config,
					uint32_t channel)
{
	struct siwx19x_gpdma_data *data = dev->data;
	int operation_width = config->source_data_size * config->source_burst_length;
	struct siwx91x_gpdma_channel_info *chan_info = &data->chan_info[channel];
	int ret;

	if (siwx91x_gpdma_xfer_dir(config->channel_direction) == SIWX91X_TRANSFER_DIR_INVALID) {
		return -EINVAL;
	}

	if (config->dest_data_size != config->source_data_size) {
		LOG_ERR("Data size mismatch");
		return -EINVAL;
	}

	if (config->dest_burst_length != config->source_burst_length) {
		LOG_ERR("Burst length mismatch");
		return -EINVAL;
	}

	if (operation_width >= SIWX91X_GPDMA_MAX_FIFO_SIZE) {
		LOG_ERR("Maximum fifo size reached: data_size × burst_length = %d >= %d",
			operation_width, SIWX91X_GPDMA_MAX_FIFO_SIZE);
		return -EINVAL;
	}

	if (siwx91x_gpdma_data_size(config->source_data_size) == SIWX91X_DATA_WIDTH_INVALID) {
		return -EINVAL;
	}

	chan_info->xfer_direction = config->channel_direction;
	data->reload_compatible = (siwx91x_gpdma_count_hw_desc(config, operation_width) == 1);

	ret = siwx91x_gpdma_desc_config(data, config, chan_info);
	if (ret) {
		return ret;
	}

	if (RSI_GPDMA_SetupChannelTransfer(&data->hal_ctx, channel, chan_info->desc)) {
		return -EIO;
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
		LOG_ERR("Invalid priority values: (valid range: 0-3)");
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

	if (size > SIWX91X_GPDMA_MAX_XFER_COUNT - data_size) {
		LOG_ERR("Maximum xfer size should be <= %d",
			SIWX91X_GPDMA_MAX_XFER_COUNT - data_size);
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

	if (data->chan_info[channel].channel_active) {
		pm_device_runtime_get(dev);
		data->chan_info[channel].channel_active = true;
	}

	if (RSI_GPDMA_DMAChannelTrigger(&data->hal_ctx, channel)) {
		if (data->chan_info[channel].channel_active) {
			pm_device_runtime_put(dev);
			data->chan_info[channel].channel_active = false;
		}
		return -EINVAL;
	}

	return 0;
}

static int siwx91x_gpdma_stop(const struct device *dev, uint32_t channel)
{
	struct siwx19x_gpdma_data *data = dev->data;

	if (channel >= data->dma_ctx.dma_channels) {
		return -EINVAL;
	}

	if (RSI_GPDMA_AbortChannel(&data->hal_ctx, channel)) {
		return -EINVAL;
	}

	siwx91x_gpdma_free_desc(data, data->chan_info[channel].desc);

	if (data->chan_info[channel].channel_active) {
		pm_device_runtime_put(dev);
		data->chan_info[channel].channel_active = false;
	}

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

static int gpdma_siwx91x_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct siwx91x_gpdma_config *cfg = dev->config;
	struct siwx19x_gpdma_data *data = dev->data;
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		break;
	case PM_DEVICE_ACTION_TURN_ON: {
		RSI_GPDMA_INIT_T gpdma_init = {
			.pUserData = NULL,
			.baseG = (uint32_t)cfg->reg,
			.baseC = (uint32_t)cfg->channel_reg,
			.sramBase = (uint32_t)data->desc_pool->buffer,
		};
		RSI_GPDMA_HANDLE_T gpdma_handle;

		ret = clock_control_on(cfg->clock_dev, cfg->clock_subsys);
		if (ret < 0 && ret != -EALREADY) {
			return ret;
		}

		gpdma_handle = RSI_GPDMA_Init(&data->hal_ctx, &gpdma_init);
		if (gpdma_handle != &data->hal_ctx) {
			return -EIO;
		}

		break;
	}
	case PM_DEVICE_ACTION_TURN_OFF:
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int siwx91x_gpdma_init(const struct device *dev)
{
	const struct siwx91x_gpdma_config *cfg = dev->config;

	cfg->irq_configure();

	return pm_device_driver_init(dev, gpdma_siwx91x_pm_action);
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

	if (channel_int_status & abort_mask) {
		RSI_GPDMA_AbortChannel(&data->hal_ctx, channel);
		cfg->reg->GLOBAL.INTERRUPT_STAT_REG = abort_mask;
		if (data->chan_info[channel].channel_active) {
			pm_device_runtime_put_async(dev, K_NO_WAIT);
			data->chan_info[channel].channel_active = false;
		}
	}

	if (channel_int_status & desc_fetch_mask) {
		cfg->reg->GLOBAL.INTERRUPT_STAT_REG = desc_fetch_mask;
		if (data->chan_info[channel].cb) {
			data->chan_info[channel].cb(dev, data->chan_info[channel].cb_data, channel,
						    0);
		}
	}

	if (channel_int_status & done_mask) {
		siwx91x_gpdma_free_desc(data, data->chan_info[channel].desc);
		data->chan_info[channel].desc = NULL;
		cfg->reg->GLOBAL.INTERRUPT_STAT_REG = done_mask;

		if (data->chan_info[channel].channel_active) {
			pm_device_runtime_put_async(dev, K_NO_WAIT);
			data->chan_info[channel].channel_active = false;
		}

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
				     CONFIG_DMA_SILABS_SIWX91X_GPDMA_DESCR_COUNT, 32);             \
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
	PM_DEVICE_DT_INST_DEFINE(inst, gpdma_siwx91x_pm_action);                                   \
	DEVICE_DT_INST_DEFINE(inst, siwx91x_gpdma_init, PM_DEVICE_DT_INST_GET(inst),               \
			      &siwx91x_gpdma_data_##inst, &siwx91x_gpdma_cfg_##inst, POST_KERNEL,  \
			      CONFIG_DMA_INIT_PRIORITY, &siwx91x_gpdma_api);

DT_INST_FOREACH_STATUS_OKAY(SIWX91X_GPDMA_INIT)
