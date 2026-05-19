/*
 * Copyright (c) 2024-2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT silabs_udma
#include <errno.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/mem_blocks.h>
#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/types.h>
#include "rsi_rom_udma.h"
#include "rsi_rom_udma_wrapper.h"
#include "rsi_udma.h"
#include "sl_status.h"

#define DMA_MAX_TRANSFER_COUNT           1024
#define DMA_CH_PRIORITY_HIGH             1
#define DMA_CH_PRIORITY_LOW              0

/* This value is missing in rsi_udma.h */
#define UDMA_MODE_PER_ALT_SCATTER_GATHER 0x07

LOG_MODULE_REGISTER(si91x_dma, CONFIG_DMA_LOG_LEVEL);

struct dma_siwx91x_channel_info {
	dma_callback_t dma_callback;        /* User callback */
	void *cb_data;                      /* User callback data */
	RSI_UDMA_DESC_T *sg_desc_addr_info; /* Scatter-Gather table start address */
	uint32_t channel_dir;
	bool channel_active;                /* Channel active flag */
};

struct dma_siwx91x_config {
	UDMA0_Type *reg;                 /* UDMA register base address */
	uint8_t irq_number;              /* IRQ number */
	RSI_UDMA_DESC_T *sram_desc_addr; /* SRAM Address for UDMA Descriptor Storage */
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	void (*irq_configure)(void);     /* IRQ configure function */
};

struct dma_siwx91x_data {
	struct dma_context dma_ctx;
	UDMA_Channel_Info *chan_info;
	struct dma_siwx91x_channel_info *zephyr_channel_info;
	struct sys_mem_blocks *dma_desc_pool; /* Pointer to the memory pool for DMA descriptor */
	RSI_UDMA_DATACONTEXT_T udma_handle;  /* Buffer to store UDMA handle
					      * related information
					      */
};

static bool siwx91x_udma_is_valid_dir(uint32_t val)
{
	switch (val) {
	case MEMORY_TO_MEMORY:
	case MEMORY_TO_PERIPHERAL:
	case PERIPHERAL_TO_MEMORY:
		return true;
	default:
		return false;
	}
}

static bool siwx91x_udma_is_valid_width(uint32_t val)
{
	switch (val) {
	case 1:
	case 2:
	case 4:
		return true;
	default:
		return false;
	}
}

static bool siwx91x_udma_is_valid_burst_len(uint32_t val)
{
	switch (val) {
	case 1:
	case 2:
	case 4:
		return true;
	default:
		return false;
	}
}

static bool siwx91x_udma_is_valid_adjustment(uint32_t val)
{
	switch (val) {
	case DMA_ADDR_ADJ_INCREMENT:
	case DMA_ADDR_ADJ_NO_CHANGE:
		return true;
	default:
		return false;
	}
}

static int siwx91x_udma_fill_block_descs(const struct dma_config *cfg,
					 const struct dma_block_config *block,
					 RSI_UDMA_DESC_T *descs)
{
	uint8_t *src_addr = (uint8_t *)block->source_address;
	uint8_t *dst_addr = (uint8_t *)block->dest_address;
	uint32_t remaining = block->block_size;
	int cnt = 0;

	do {
		uint32_t chunk = MIN(remaining, DMA_MAX_TRANSFER_COUNT * cfg->source_burst_length);
		RSI_UDMA_CHA_CONFIG_DATA_T *desc_cfg =
			(RSI_UDMA_CHA_CONFIG_DATA_T *)&descs->vsUDMAChaConfigData1;

		desc_cfg->srcSize = find_lsb_set(cfg->source_burst_length) - 1;
		desc_cfg->dstSize = find_lsb_set(cfg->dest_burst_length) - 1;
		desc_cfg->totalNumOfDMATrans = (chunk / cfg->source_burst_length) - 1;

		if (block->source_addr_adj == DMA_ADDR_ADJ_NO_CHANGE) {
			descs->pSrcEndAddr = src_addr;
		} else {
			descs->pSrcEndAddr = src_addr + chunk - cfg->source_burst_length;
		}

		if (block->dest_addr_adj == DMA_ADDR_ADJ_NO_CHANGE) {
			descs->pDstEndAddr = dst_addr;
		} else {
			descs->pDstEndAddr = dst_addr + chunk - cfg->dest_burst_length;
		}

		/* Set the transfer type based on whether it is a peripheral request */
		if (cfg->channel_direction == MEMORY_TO_MEMORY) {
			desc_cfg->transferType = UDMA_MODE_MEM_ALT_SCATTER_GATHER;
		} else {
			desc_cfg->transferType = UDMA_MODE_PER_ALT_SCATTER_GATHER;
		}

		desc_cfg->rPower = ARBSIZE_1;

		if (block->source_addr_adj == DMA_ADDR_ADJ_NO_CHANGE) {
			desc_cfg->srcInc = UDMA_SRC_INC_NONE;
		} else {
			desc_cfg->srcInc = find_lsb_set(cfg->source_burst_length) - 1;
		}

		if (block->dest_addr_adj == DMA_ADDR_ADJ_NO_CHANGE) {
			desc_cfg->dstInc = UDMA_DST_INC_NONE;
		} else {
			desc_cfg->dstInc = find_lsb_set(cfg->dest_burst_length) - 1;
		}

		remaining -= chunk;
		if (block->source_addr_adj != DMA_ADDR_ADJ_NO_CHANGE) {
			src_addr += chunk;
		}
		if (block->dest_addr_adj != DMA_ADDR_ADJ_NO_CHANGE) {
			dst_addr += chunk;
		}
		descs++;
		cnt++;
	} while (remaining > 0);

	return cnt;
}

static int siwx91x_udma_fill_sg_descs(const struct dma_config *config_zephyr,
				      RSI_UDMA_DESC_T *desc_base, uint32_t desc_count)
{
	const struct dma_block_config *block = config_zephyr->head_block;
	int cnt = 0;

	for (int i = 0; i < config_zephyr->block_count; i++) {
		if (!siwx91x_udma_is_valid_adjustment(block->source_addr_adj) ||
		    !siwx91x_udma_is_valid_adjustment(block->dest_addr_adj)) {
			return -EINVAL;
		}
		if (block->block_size % config_zephyr->source_burst_length) {
			return -EINVAL;
		}

		cnt += siwx91x_udma_fill_block_descs(config_zephyr, block, desc_base + cnt);
		block = block->next_block;
	}
	__ASSERT(cnt == desc_count, "Corrupted state");

	if (block != NULL) {
		return -EINVAL;
	}

	/* Set the transfer type for the last descriptor */
	if (config_zephyr->channel_direction == MEMORY_TO_MEMORY) {
		desc_base[cnt - 1].vsUDMAChaConfigData1.transferType = UDMA_MODE_AUTO;
	} else {
		desc_base[cnt - 1].vsUDMAChaConfigData1.transferType = UDMA_MODE_BASIC;
	}

	return 0;
}

static uint32_t siwx91x_udma_count_hw_desc(const struct dma_config *config)
{
	const struct dma_block_config *block = config->head_block;
	uint32_t max_chunk = DMA_MAX_TRANSFER_COUNT * config->source_burst_length;
	uint32_t total = 0;

	for (int i = 0; i < config->block_count; i++) {
		if (block->block_size) {
			total += DIV_ROUND_UP(block->block_size, max_chunk);
		} else {
			total += 1;
		}
		block = block->next_block;
	}

	return total;
}

/* Configure DMA for scatter-gather transfer */
static int siwx91x_udma_config_sg(const struct device *dev, RSI_UDMA_HANDLE_T udma_handle,
				  uint32_t channel, const struct dma_config *config)
{
	const struct dma_siwx91x_config *cfg = dev->config;
	struct dma_siwx91x_data *data = dev->data;
	uint32_t desc_count = siwx91x_udma_count_hw_desc(config);
	RSI_UDMA_DESC_T *desc_base = NULL;
	uint8_t transfer_type;
	int ret;

	ret = sys_mem_blocks_alloc_contiguous(data->dma_desc_pool, desc_count,
					    (void **)&desc_base);
	if (ret) {
		return -EINVAL;
	}

	ret = siwx91x_udma_fill_sg_descs(config, desc_base, desc_count);
	if (ret) {
		sys_mem_blocks_free_contiguous(data->dma_desc_pool, (void *)desc_base, desc_count);
		return -EINVAL;
	}

	/* This channel information is used to distinguish scatter-gather transfers and
	 * free the allocated descriptors in sg_transfer_desc_block
	 */
	data->chan_info[channel].Cnt = desc_count;
	data->zephyr_channel_info[channel].sg_desc_addr_info = desc_base;

	/* Store the transfer direction. This is used to trigger SW request for
	 * Memory to Memory transfers.
	 */
	data->zephyr_channel_info[channel].channel_dir = config->channel_direction;

	RSI_UDMA_InterruptClear(udma_handle, channel);
	RSI_UDMA_ErrorStatusClear(udma_handle);

	if (cfg->reg == UDMA0) {
		/* UDMA0 is accessible by both TA and M4, so an interrupt should be configured in
		 * the TA-M4 common register set to signal the TA when UDMA0 is actively in use.
		 */
		sys_write32((BIT(channel) | M4SS_UDMA_INTR_SEL), (mem_addr_t)&M4SS_UDMA_INTR_SEL);
	} else {
		sys_set_bit((mem_addr_t)&cfg->reg->UDMA_INTR_MASK_REG, channel);
	}

	sys_write32(BIT(channel), (mem_addr_t)&cfg->reg->CHNL_PRI_ALT_SET);
	sys_write32(BIT(channel), (mem_addr_t)&cfg->reg->CHNL_REQ_MASK_CLR);

	if (config->channel_direction == MEMORY_TO_MEMORY) {
		transfer_type = UDMA_MODE_MEM_SCATTER_GATHER;
	} else {
		transfer_type = UDMA_MODE_PER_SCATTER_GATHER;
	}
	RSI_UDMA_SetChannelScatterGatherTransfer(udma_handle, channel, desc_count,
						 desc_base, transfer_type);
	return 0;
}

static int siwx91x_udma_config_single(const struct device *dev, RSI_UDMA_HANDLE_T udma_handle,
				      uint32_t channel, const struct dma_config *config)
{
	uint32_t dma_transfer_num = config->head_block->block_size / config->source_burst_length;
	const struct dma_siwx91x_config *cfg = dev->config;
	struct dma_siwx91x_data *data = dev->data;
	UDMA_RESOURCES udma_resources = {
		.reg = cfg->reg,
		.udma_irq_num = cfg->irq_number,
		/* SRAM address where UDMA descriptor is stored */
		.desc = cfg->sram_desc_addr,
	};
	RSI_UDMA_CHA_CONFIG_DATA_T channel_control = {
		.transferType = UDMA_MODE_BASIC,
	};
	RSI_UDMA_CHA_CFG_T channel_config = {};
	int status;

	channel_config.channelPrioHigh = config->channel_priority;
	channel_config.dmaCh = channel;

	if (config->channel_direction == MEMORY_TO_MEMORY) {
		channel_control.rPower = ARBSIZE_1024;
		channel_config.periphReq = 0;
	} else {
		channel_control.rPower = ARBSIZE_1;
		channel_config.periphReq = 1;
	}

	/* Obtain the number of transfers */
	if (dma_transfer_num >= DMA_MAX_TRANSFER_COUNT) {
		/* Maximum number of transfers is 1024 */
		channel_control.totalNumOfDMATrans = DMA_MAX_TRANSFER_COUNT - 1;
	} else {
		channel_control.totalNumOfDMATrans = dma_transfer_num;
	}

	channel_control.srcSize = find_lsb_set(config->source_burst_length) - 1;
	channel_control.dstSize = find_lsb_set(config->dest_burst_length) - 1;

	if (!siwx91x_udma_is_valid_adjustment(config->head_block->source_addr_adj) ||
	    !siwx91x_udma_is_valid_adjustment(config->head_block->dest_addr_adj)) {
		return -EINVAL;
	}

	if (config->head_block->source_addr_adj == DMA_ADDR_ADJ_NO_CHANGE) {
		channel_control.srcInc = UDMA_SRC_INC_NONE;
	} else {
		channel_control.srcInc = find_lsb_set(config->source_burst_length) - 1;
	}

	if (config->head_block->dest_addr_adj == DMA_ADDR_ADJ_NO_CHANGE) {
		channel_control.dstInc = UDMA_DST_INC_NONE;
	} else {
		channel_control.dstInc = find_lsb_set(config->dest_burst_length) - 1;
	}

	/* Clear the CHNL_PRI_ALT_CLR to use primary DMA descriptor structure */
	sys_write32(BIT(channel), (mem_addr_t)&cfg->reg->CHNL_PRI_ALT_CLR);

	status = UDMAx_ChannelConfigure(&udma_resources, (uint8_t)channel,
					config->head_block->source_address,
					config->head_block->dest_address,
					dma_transfer_num, channel_control,
					&channel_config, NULL, data->chan_info,
					udma_handle);
	if (status) {
		return -EIO;
	}

	/* Store the transfer direction. This is used to trigger SW request for
	 * Memory to Memory transfers.
	 */
	data->zephyr_channel_info[channel].channel_dir = config->channel_direction;

	return 0;
}

/* Function to configure UDMA channel for transfer */
static int siwx91x_dma_configure(const struct device *dev, uint32_t channel,
				 struct dma_config *config)
{
	struct dma_siwx91x_data *data = dev->data;
	void *udma_handle = &data->udma_handle;
	int status;

	/* Expecting a fixed channel number between 0-31 for dma0 and 0-11 for ulpdma */
	if (channel >= data->dma_ctx.dma_channels) {
		return -EINVAL;
	}

	/* Disable the channel before configuring */
	if (RSI_UDMA_ChannelDisable(udma_handle, channel) != 0) {
		return -EIO;
	}

	if (config->channel_priority != DMA_CH_PRIORITY_LOW &&
	    config->channel_priority != DMA_CH_PRIORITY_HIGH) {
		return -EINVAL;
	}

	if (config->cyclic || config->complete_callback_en) {
		return -EINVAL;
	}

	if (!siwx91x_udma_is_valid_dir(config->channel_direction)) {
		return -EINVAL;
	}

	if (!siwx91x_udma_is_valid_width(config->source_data_size) ||
	    !siwx91x_udma_is_valid_width(config->dest_data_size)) {
		return -EINVAL;
	}

	if (!siwx91x_udma_is_valid_burst_len(config->source_burst_length) ||
	    !siwx91x_udma_is_valid_burst_len(config->dest_burst_length)) {
		return -EINVAL;
	}

	if (config->head_block->next_block != NULL) {
		status = siwx91x_udma_config_sg(dev, udma_handle, channel, config);
	} else {
		status = siwx91x_udma_config_single(dev, udma_handle, channel, config);
	}
	if (status) {
		return status;
	}

	data->zephyr_channel_info[channel].dma_callback = config->dma_callback;
	data->zephyr_channel_info[channel].cb_data = config->user_data;

	atomic_set_bit(data->dma_ctx.atomic, channel);

	return 0;
}

/* Function to reload UDMA channel for new transfer */
static int siwx91x_dma_reload(const struct device *dev, uint32_t channel, uint32_t src,
			      uint32_t dst, size_t size)
{
	const struct dma_siwx91x_config *cfg = dev->config;
	struct dma_siwx91x_data *data = dev->data;
	void *udma_handle = &data->udma_handle;
	void *desc_src_addr;
	void *desc_dst_addr;
	uint32_t length;
	RSI_UDMA_DESC_T *udma_table = cfg->sram_desc_addr;
	uint8_t xfer_size = 1 << udma_table[channel].vsUDMAChaConfigData1.srcSize;

	/* Expecting a fixed channel number between 0-31 for dma0 and 0-11 for ulpdma */
	if (channel >= data->dma_ctx.dma_channels) {
		return -EINVAL;
	}

	/* Disable the channel before reloading transfer */
	if (RSI_UDMA_ChannelDisable(udma_handle, channel) != 0) {
		return -EIO;
	}

	/* Update new channel info to dev->data structure */
	data->chan_info[channel].SrcAddr = src;
	data->chan_info[channel].DestAddr = dst;
	data->chan_info[channel].Size = size / xfer_size;

	/* Update new transfer size to dev->data structure */
	if (data->chan_info[channel].Size >= DMA_MAX_TRANSFER_COUNT) {
		data->chan_info[channel].Cnt = DMA_MAX_TRANSFER_COUNT;
	} else {
		data->chan_info[channel].Cnt = size / xfer_size;
	}

	/* Program the DMA descriptors with new transfer data information. */
	if (udma_table[channel].vsUDMAChaConfigData1.srcInc != UDMA_SRC_INC_NONE) {
		length = data->chan_info[channel].Cnt
			 << udma_table[channel].vsUDMAChaConfigData1.srcInc;
		desc_src_addr = (void *)(src + length - 1);
		udma_table[channel].pSrcEndAddr = desc_src_addr;
	}

	if (udma_table[channel].vsUDMAChaConfigData1.dstInc != UDMA_SRC_INC_NONE) {
		length = data->chan_info[channel].Cnt
			 << udma_table[channel].vsUDMAChaConfigData1.dstInc;
		desc_dst_addr = (void *)(dst + length - 1);
		udma_table[channel].pDstEndAddr = desc_dst_addr;
	}

	udma_table[channel].vsUDMAChaConfigData1.totalNumOfDMATrans =
		data->chan_info[channel].Cnt - 1;
	udma_table[channel].vsUDMAChaConfigData1.transferType = UDMA_MODE_BASIC;

	return 0;
}

/* Function to start a DMA transfer */
static int siwx91x_dma_start(const struct device *dev, uint32_t channel)
{
	const struct dma_siwx91x_config *cfg = dev->config;
	struct dma_siwx91x_data *data = dev->data;
	void *udma_handle = &data->udma_handle;

	/* Expecting a fixed channel number between 0-31 for dma0 and 0-11 for ulpdma */
	if (channel >= data->dma_ctx.dma_channels) {
		return -EINVAL;
	}

	if (!data->zephyr_channel_info[channel].channel_active) {
		pm_device_runtime_get(dev);
		data->zephyr_channel_info[channel].channel_active = true;
	}

	if (RSI_UDMA_ChannelEnable(udma_handle, channel) != 0) {
		if (data->zephyr_channel_info[channel].channel_active) {
			pm_device_runtime_put(dev);
			data->zephyr_channel_info[channel].channel_active = false;
		}
		return -EINVAL;
	}

	/* Check if the transfer type is memory-memory */
	if (data->zephyr_channel_info[channel].channel_dir == MEMORY_TO_MEMORY) {
		/* Apply software trigger to start transfer */
		sys_set_bit((mem_addr_t)&cfg->reg->CHNL_SW_REQUEST, channel);
	}

	return 0;
}

/* Function to stop a DMA transfer */
static int siwx91x_dma_stop(const struct device *dev, uint32_t channel)
{
	struct dma_siwx91x_data *data = dev->data;
	void *udma_handle = &data->udma_handle;

	/* Expecting a fixed channel number between 0-31 for dma0 and 0-11 for ulpdma */
	if (channel >= data->dma_ctx.dma_channels) {
		return -EINVAL;
	}

	if (RSI_UDMA_ChannelDisable(udma_handle, channel) != 0) {
		return -EIO;
	}

	if (data->zephyr_channel_info[channel].channel_active) {
		pm_device_runtime_put(dev);
		data->zephyr_channel_info[channel].channel_active = false;
	}

	return 0;
}

/* Function to fetch DMA channel status */
static int siwx91x_dma_get_status(const struct device *dev, uint32_t channel,
				  struct dma_status *stat)
{
	const struct dma_siwx91x_config *cfg = dev->config;
	struct dma_siwx91x_data *data = dev->data;
	RSI_UDMA_DESC_T *udma_table = cfg->sram_desc_addr;

	/* Expecting a fixed channel number between 0-31 for dma0 and 0-11 for ulpdma */
	if (channel >= data->dma_ctx.dma_channels) {
		return -EINVAL;
	}

	if (!atomic_test_bit(data->dma_ctx.atomic, channel)) {
		return -EINVAL;
	}

	/* Read the channel status register */
	stat->busy = sys_test_bit((mem_addr_t)&cfg->reg->CHANNEL_STATUS_REG, channel);

	/* Obtain the transfer direction from channel descriptors */
	if (udma_table[channel].vsUDMAChaConfigData1.srcInc == UDMA_SRC_INC_NONE) {
		stat->dir = PERIPHERAL_TO_MEMORY;
	} else if (udma_table[channel].vsUDMAChaConfigData1.dstInc == UDMA_DST_INC_NONE) {
		stat->dir = MEMORY_TO_PERIPHERAL;
	} else {
		stat->dir = MEMORY_TO_MEMORY;
	}

	return 0;
}

bool siwx91x_dma_chan_filter(const struct device *dev, int channel, void *filter_param)
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

static int dma_siwx91x_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct dma_siwx91x_config *cfg = dev->config;
	struct dma_siwx91x_data *data = dev->data;
	void *udma_handle = NULL;
	UDMA_RESOURCES udma_resources = {
		.reg = cfg->reg, /* UDMA register base address */
		.udma_irq_num = cfg->irq_number,
		.desc = cfg->sram_desc_addr,
	};
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		break;
	case PM_DEVICE_ACTION_TURN_ON:
		ret = clock_control_on(cfg->clock_dev, cfg->clock_subsys);
		if (ret < 0 && ret != -EALREADY) {
			return ret;
		}

		udma_handle = UDMAx_Initialize(&udma_resources, udma_resources.desc, NULL,
					       (uint32_t *)&data->udma_handle);
		if (udma_handle != &data->udma_handle) {
			return -EINVAL;
		}

		if (UDMAx_DMAEnable(&udma_resources, udma_handle) != 0) {
			return -EBUSY;
		}
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

/* Function to initialize DMA peripheral */
static int siwx91x_dma_init(const struct device *dev)
{
	const struct dma_siwx91x_config *cfg = dev->config;

	/* Connect the DMA interrupt */
	cfg->irq_configure();

	return pm_device_driver_init(dev, dma_siwx91x_pm_action);
}

static void siwx91x_dma_isr(const struct device *dev)
{
	const struct dma_siwx91x_config *cfg = dev->config;
	struct dma_siwx91x_data *data = dev->data;
	UDMA_RESOURCES udma_resources = {
		.reg = cfg->reg,
		.udma_irq_num = cfg->irq_number,
		.desc = cfg->sram_desc_addr,
	};
	uint8_t channel;

	/* Disable the IRQ to prevent the ISR from being triggered by
	 * interrupts from other DMA channels.
	 */
	irq_disable(cfg->irq_number);

	channel = find_lsb_set(cfg->reg->UDMA_DONE_STATUS_REG);
	/* Identify the interrupt channel */
	if (!channel || channel > data->dma_ctx.dma_channels) {
		goto out;
	}
	/* find_lsb_set() returns 1 indexed value */
	channel -= 1;

	if (data->zephyr_channel_info[channel].sg_desc_addr_info) {
		/* A Scatter-Gather transfer is completed, free the allocated descriptors */
		if (sys_mem_blocks_free_contiguous(
			    data->dma_desc_pool,
			    (void *)data->zephyr_channel_info[channel].sg_desc_addr_info,
			    data->chan_info[channel].Cnt)) {
			sys_write32(BIT(channel), (mem_addr_t)&cfg->reg->UDMA_DONE_STATUS_REG);
			goto out;
		}
		data->chan_info[channel].Cnt = 0;
		data->chan_info[channel].Size = 0;
		data->zephyr_channel_info[channel].sg_desc_addr_info = NULL;
	}

	if (data->chan_info[channel].Cnt == data->chan_info[channel].Size) {
		sys_write32(BIT(channel), (mem_addr_t)&cfg->reg->UDMA_DONE_STATUS_REG);
		if (data->zephyr_channel_info[channel].channel_active) {
			pm_device_runtime_put_async(dev, K_NO_WAIT);
			data->zephyr_channel_info[channel].channel_active = false;
		}
		if (data->zephyr_channel_info[channel].dma_callback) {
			/* Transfer complete, call user callback */
			data->zephyr_channel_info[channel].dma_callback(
				dev, data->zephyr_channel_info[channel].cb_data, channel, 0);
		}
	} else {
		/* Call UDMA ROM IRQ handler. */
		ROMAPI_UDMA_WRAPPER_API->uDMAx_IRQHandler(&udma_resources, udma_resources.desc,
							  data->chan_info);
		/* Is a Memory-to-memory Transfer */
		if (udma_resources.desc[channel].vsUDMAChaConfigData1.srcInc != UDMA_SRC_INC_NONE &&
		    udma_resources.desc[channel].vsUDMAChaConfigData1.dstInc != UDMA_DST_INC_NONE) {
			/* Set the software trigger bit for starting next transfer */
			sys_set_bit((mem_addr_t)&cfg->reg->CHNL_SW_REQUEST, channel);
		}
	}

out:
	/* Enable the IRQ to restore interrupt functionality for other DMA channels */
	irq_enable(cfg->irq_number);
}

/* Store the Si91x DMA APIs */
static DEVICE_API(dma, siwx91x_dma_api) = {
	.config = siwx91x_dma_configure,
	.reload = siwx91x_dma_reload,
	.start = siwx91x_dma_start,
	.stop = siwx91x_dma_stop,
	.get_status = siwx91x_dma_get_status,
	.chan_filter = siwx91x_dma_chan_filter,
};

#define SIWX91X_DMA_INIT(inst)                                                                     \
	static ATOMIC_DEFINE(dma_channels_atomic_##inst, DT_INST_PROP(inst, dma_channels));        \
	static UDMA_Channel_Info dma_channel_info_##inst[DT_INST_PROP(inst, dma_channels)];        \
	SYS_MEM_BLOCKS_DEFINE_STATIC_TYPE(desc_pool_##inst, RSI_UDMA_DESC_T,                       \
					  CONFIG_DMA_SILABS_SIWX91X_UDMA_DESCR_COUNT);             \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, silabs_sram_region),                               \
		    (),                                                                            \
		    (static __aligned(1024) RSI_UDMA_DESC_T                                        \
			     siwx91x_dma_chan_desc##inst[DT_INST_PROP(inst, dma_channels) * 2];))  \
	static struct dma_siwx91x_channel_info                                                     \
		zephyr_channel_info_##inst[DT_INST_PROP(inst, dma_channels)];                      \
	static struct dma_siwx91x_data dma_data_##inst = {                                         \
		.dma_ctx.magic = DMA_MAGIC,                                                        \
		.dma_ctx.dma_channels = DT_INST_PROP(inst, dma_channels),                          \
		.dma_ctx.atomic = dma_channels_atomic_##inst,                                      \
		.chan_info = dma_channel_info_##inst,                                              \
		.zephyr_channel_info = zephyr_channel_info_##inst,                                 \
		.dma_desc_pool = &desc_pool_##inst,                                                \
	};                                                                                         \
	static void siwx91x_dma_irq_configure_##inst(void)                                         \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ(inst, irq), DT_INST_IRQ(inst, priority), siwx91x_dma_isr,  \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
		irq_enable(DT_INST_IRQ(inst, irq));                                                \
	}                                                                                          \
	static const struct dma_siwx91x_config dma_cfg_##inst = {                                  \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                             \
		.clock_subsys = (clock_control_subsys_t)DT_INST_PHA(inst, clocks, clkid),          \
		.reg = (UDMA0_Type *)DT_INST_REG_ADDR(inst),                                       \
		.irq_number = DT_INST_PROP_BY_IDX(inst, interrupts, 0),                            \
		.sram_desc_addr = COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, silabs_sram_region),     \
					      ((RSI_UDMA_DESC_T *)DT_REG_ADDR(DT_INST_PHANDLE(inst, silabs_sram_region))), \
					      (siwx91x_dma_chan_desc##inst)),                      \
		.irq_configure = siwx91x_dma_irq_configure_##inst,                                 \
	};                                                                                         \
	PM_DEVICE_DT_INST_DEFINE(inst, dma_siwx91x_pm_action);                                     \
	DEVICE_DT_INST_DEFINE(inst, siwx91x_dma_init,  PM_DEVICE_DT_INST_GET(inst),                \
			      &dma_data_##inst, &dma_cfg_##inst, POST_KERNEL,                      \
			      CONFIG_DMA_INIT_PRIORITY,                                           \
			      &siwx91x_dma_api);

DT_INST_FOREACH_STATUS_OKAY(SIWX91X_DMA_INIT)
