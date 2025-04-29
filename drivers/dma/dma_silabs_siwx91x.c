/*
 * Copyright (c) 2024 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/mem_blocks.h>
#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>
#include <zephyr/types.h>
#include "rsi_rom_udma.h"
#include "rsi_rom_udma_wrapper.h"
#include "rsi_udma.h"
#include "sl_status.h"

#define DT_DRV_COMPAT                    silabs_siwx91x_dma
#define DMA_MAX_TRANSFER_COUNT           1024
#define DMA_CH_PRIORITY_HIGH             1
#define DMA_CH_PRIORITY_LOW              0
#define UDMA_ADDR_INC_NONE               0x03
#define UDMA_MODE_PER_ALT_SCATTER_GATHER 0x07

LOG_MODULE_REGISTER(si91x_dma, CONFIG_DMA_LOG_LEVEL);

enum dma_xfer_dir {
	TRANSFER_MEM_TO_MEM,
	TRANSFER_TO_OR_FROM_PER,
	TRANSFER_DIR_INVALID = -1,
};

struct dma_siwx91x_channel_info {
	dma_callback_t dma_callback;        /* User callback */
	void *cb_data;                      /* User callback data */
	RSI_UDMA_DESC_T *sg_desc_addr_info; /* Scatter-Gather table start address */
	enum dma_xfer_dir xfer_direction;   /* mem<->mem ot per<->mem */
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

static enum dma_xfer_dir siwx91x_transfer_direction(uint32_t dir)
{
	if (dir == MEMORY_TO_MEMORY) {
		return TRANSFER_MEM_TO_MEM;
	}

	if (dir == MEMORY_TO_PERIPHERAL || dir == PERIPHERAL_TO_MEMORY) {
		return TRANSFER_TO_OR_FROM_PER;
	}

	return TRANSFER_DIR_INVALID;
}

static bool siwx91x_is_data_width_valid(uint32_t data_width)
{
	switch (data_width) {
	case 1:
	case 2:
	case 4:
		return true;
	default:
		return false;
	}
}

static int siwx91x_burst_length(uint32_t blen)
{
	switch (blen) {
	case 1:
		return SRC_INC_8;
	case 2:
		return SRC_INC_16;
	case 4:
		return SRC_INC_32;
	default:
		return -EINVAL;
	}
}

static int siwx91x_addr_adjustment(uint32_t adjustment)
{
	switch (adjustment) {
	case 0:
		return 0; /* Addr Increment */
	case 2:
		return UDMA_ADDR_INC_NONE; /* No Address increment */
	default:
		return -EINVAL;
	}
}

/* Sets up the scatter-gather descriptor table for a DMA transfer */
static int siwx91x_sg_fill_desc(RSI_UDMA_DESC_T *descs, const struct dma_config *config_zephyr)
{
	const struct dma_block_config *block_addr = config_zephyr->head_block;
	RSI_UDMA_CHA_CONFIG_DATA_T *cfg_91x;

	for (int i = 0; i < config_zephyr->block_count; i++) {
		sys_write32((uint32_t)&descs[i].vsUDMAChaConfigData1, (mem_addr_t)&cfg_91x);

		if (siwx91x_addr_adjustment(block_addr->source_addr_adj) == UDMA_ADDR_INC_NONE) {
			descs[i].pSrcEndAddr = (void *)block_addr->source_address;
		} else {
			descs[i].pSrcEndAddr = (void *)(block_addr->source_address +
							(block_addr->block_size -
							 config_zephyr->source_burst_length));
		}
		if (siwx91x_addr_adjustment(block_addr->dest_addr_adj) == UDMA_ADDR_INC_NONE) {
			descs[i].pDstEndAddr = (void *)block_addr->dest_address;
		} else {
			descs[i].pDstEndAddr = (void *)(block_addr->dest_address +
							(block_addr->block_size -
							 config_zephyr->dest_burst_length));
		}

		cfg_91x->srcSize = siwx91x_burst_length(config_zephyr->source_burst_length);
		cfg_91x->dstSize = siwx91x_burst_length(config_zephyr->dest_burst_length);

		/* Calculate the number of DMA transfers required */
		if (block_addr->block_size / config_zephyr->source_burst_length >
		    DMA_MAX_TRANSFER_COUNT) {
			return -EINVAL;
		}

		cfg_91x->totalNumOfDMATrans =
			block_addr->block_size / config_zephyr->source_burst_length - 1;

		/* Set the transfer type based on whether it is a peripheral request */
		if (siwx91x_transfer_direction(config_zephyr->channel_direction) ==
		    TRANSFER_TO_OR_FROM_PER) {
			cfg_91x->transferType = UDMA_MODE_PER_ALT_SCATTER_GATHER;
		} else {
			cfg_91x->transferType = UDMA_MODE_MEM_ALT_SCATTER_GATHER;
		}

		cfg_91x->rPower = ARBSIZE_1;

		if (siwx91x_addr_adjustment(block_addr->source_addr_adj) < 0 ||
		    siwx91x_addr_adjustment(block_addr->dest_addr_adj) < 0) {
			return -EINVAL;
		}

		if (siwx91x_addr_adjustment(block_addr->source_addr_adj) == UDMA_ADDR_INC_NONE) {
			cfg_91x->srcInc = UDMA_SRC_INC_NONE;
		} else {
			cfg_91x->srcInc = siwx91x_burst_length(config_zephyr->source_burst_length);
		}

		if (siwx91x_addr_adjustment(block_addr->dest_addr_adj) == UDMA_ADDR_INC_NONE) {
			cfg_91x->dstInc = UDMA_DST_INC_NONE;
		} else {
			cfg_91x->dstInc = siwx91x_burst_length(config_zephyr->dest_burst_length);
		}

		/* Move to the next block */
		block_addr = block_addr->next_block;
	}

	if (block_addr != NULL) {
		/* next_block address for last block must be null */
		return -EINVAL;
	}

	/* Set the transfer type for the last descriptor */
	switch (siwx91x_transfer_direction(config_zephyr->channel_direction)) {
	case TRANSFER_TO_OR_FROM_PER:
		descs[config_zephyr->block_count - 1].vsUDMAChaConfigData1.transferType =
			UDMA_MODE_BASIC;
		break;
	case TRANSFER_MEM_TO_MEM:
		descs[config_zephyr->block_count - 1].vsUDMAChaConfigData1.transferType =
			UDMA_MODE_AUTO;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/* Configure DMA for scatter-gather transfer */
static int siwx91x_sg_chan_config(const struct device *dev, RSI_UDMA_HANDLE_T udma_handle,
				  uint32_t channel, const struct dma_config *config)
{
	const struct dma_siwx91x_config *cfg = dev->config;
	struct dma_siwx91x_data *data = dev->data;
	RSI_UDMA_DESC_T *sg_desc_base_addr = NULL;
	uint8_t transfer_type;
	enum dma_xfer_dir xfer_dir;

	xfer_dir = siwx91x_transfer_direction(config->channel_direction);
	if (xfer_dir == TRANSFER_DIR_INVALID) {
		return -EINVAL;
	}
	if (xfer_dir == TRANSFER_TO_OR_FROM_PER) {
		transfer_type = UDMA_MODE_PER_SCATTER_GATHER;
	} else {
		transfer_type = UDMA_MODE_MEM_SCATTER_GATHER;
	}

	if (!siwx91x_is_data_width_valid(config->source_data_size) ||
	    !siwx91x_is_data_width_valid(config->dest_data_size)) {
		return -EINVAL;
	}

	if (siwx91x_burst_length(config->source_burst_length) < 0 ||
	    siwx91x_burst_length(config->dest_burst_length) < 0) {
		return -EINVAL;
	}

	/* Request start index for scatter-gather descriptor table */
	if (sys_mem_blocks_alloc_contiguous(data->dma_desc_pool, config->block_count,
					    (void **)&sg_desc_base_addr)) {
		return -EINVAL;
	}

	if (siwx91x_sg_fill_desc(sg_desc_base_addr, config)) {
		return -EINVAL;
	}

	/* This channel information is used to distinguish scatter-gather transfers and
	 * free the allocated descriptors in sg_transfer_desc_block
	 */
	data->chan_info[channel].Cnt = config->block_count;
	data->zephyr_channel_info[channel].sg_desc_addr_info = sg_desc_base_addr;

	/* Store the transfer direction. This is used to trigger SW request for
	 * Memory to Memory transfers.
	 */
	data->zephyr_channel_info[channel].xfer_direction = xfer_dir;

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

	RSI_UDMA_SetChannelScatterGatherTransfer(udma_handle, channel, config->block_count,
						 sg_desc_base_addr, transfer_type);
	return 0;
}

static int siwx91x_direct_chan_config(const struct device *dev, RSI_UDMA_HANDLE_T udma_handle,
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
	enum dma_xfer_dir xfer_dir;
	int status;

	xfer_dir = siwx91x_transfer_direction(config->channel_direction);
	if (xfer_dir == TRANSFER_DIR_INVALID) {
		return -EINVAL;
	}

	channel_config.channelPrioHigh = config->channel_priority;
	channel_config.periphReq = siwx91x_transfer_direction(config->channel_direction);
	channel_config.dmaCh = channel;

	if (channel_config.periphReq) {
		/* Arbitration power for peripheral<->memory transfers */
		channel_control.rPower = ARBSIZE_1;
	} else {
		/* Arbitration power for mem-mem transfers */
		channel_control.rPower = ARBSIZE_1024;
	}

	/* Obtain the number of transfers */
	if (dma_transfer_num >= DMA_MAX_TRANSFER_COUNT) {
		/* Maximum number of transfers is 1024 */
		channel_control.totalNumOfDMATrans = DMA_MAX_TRANSFER_COUNT - 1;
	} else {
		channel_control.totalNumOfDMATrans = dma_transfer_num;
	}

	if (!siwx91x_is_data_width_valid(config->source_data_size) ||
	    !siwx91x_is_data_width_valid(config->dest_data_size)) {
		return -EINVAL;
	}
	if (siwx91x_burst_length(config->source_burst_length) < 0 ||
	    siwx91x_burst_length(config->dest_burst_length) < 0) {
		return -EINVAL;
	}

	channel_control.srcSize = siwx91x_burst_length(config->source_burst_length);
	channel_control.dstSize = siwx91x_burst_length(config->dest_burst_length);
	if (siwx91x_addr_adjustment(config->head_block->source_addr_adj) < 0 ||
	    siwx91x_addr_adjustment(config->head_block->dest_addr_adj) < 0) {
		return -EINVAL;
	}

	if (siwx91x_addr_adjustment(config->head_block->source_addr_adj) == 0) {
		channel_control.srcInc = siwx91x_burst_length(config->source_burst_length);
	} else {
		channel_control.srcInc = UDMA_SRC_INC_NONE;
	}

	if (siwx91x_addr_adjustment(config->head_block->dest_addr_adj) == 0) {
		channel_control.dstInc = siwx91x_burst_length(config->dest_burst_length);
	} else {
		channel_control.dstInc = UDMA_DST_INC_NONE;
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
	data->zephyr_channel_info[channel].xfer_direction = xfer_dir;

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
		/* Cyclic DMA feature and completion callback for each block
		 * is not supported
		 */
		return -EINVAL;
	}

	/* Configure dma channel for transfer */
	if (config->head_block->next_block != NULL) {
		/* Configure DMA for a Scatter-Gather transfer */
		status = siwx91x_sg_chan_config(dev, udma_handle, channel, config);
	} else {
		status = siwx91x_direct_chan_config(dev, udma_handle, channel, config);
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
	uint32_t desc_src_addr;
	uint32_t desc_dst_addr;
	uint32_t length;
	RSI_UDMA_DESC_T *udma_table = cfg->sram_desc_addr;

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
	data->chan_info[channel].Size = size;

	/* Update new transfer size to dev->data structure */
	if (size >= DMA_MAX_TRANSFER_COUNT) {
		data->chan_info[channel].Cnt = DMA_MAX_TRANSFER_COUNT - 1;
	} else {
		data->chan_info[channel].Cnt = size;
	}

	/* Program the DMA descriptors with new transfer data information. */
	if (udma_table[channel].vsUDMAChaConfigData1.srcInc != UDMA_SRC_INC_NONE) {
		length = data->chan_info[channel].Cnt
			 << udma_table[channel].vsUDMAChaConfigData1.srcInc;
		desc_src_addr = src + (length - 1);
		udma_table[channel].pSrcEndAddr = (void *)desc_src_addr;
	}

	if (udma_table[channel].vsUDMAChaConfigData1.dstInc != UDMA_SRC_INC_NONE) {
		length = data->chan_info[channel].Cnt
			 << udma_table[channel].vsUDMAChaConfigData1.dstInc;
		desc_dst_addr = dst + (length - 1);
		udma_table[channel].pDstEndAddr = (void *)desc_dst_addr;
	}

	udma_table[channel].vsUDMAChaConfigData1.totalNumOfDMATrans = data->chan_info[channel].Cnt;
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

	if (RSI_UDMA_ChannelEnable(udma_handle, channel) != 0) {
		return -EINVAL;
	}

	/* Check if the transfer type is memory-memory */
	if (data->zephyr_channel_info[channel].xfer_direction == TRANSFER_MEM_TO_MEM) {
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

/* Function to initialize DMA peripheral */
static int siwx91x_dma_init(const struct device *dev)
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

	ret = clock_control_on(cfg->clock_dev, cfg->clock_subsys);
	if (ret) {
		return ret;
	}

	udma_handle = UDMAx_Initialize(&udma_resources, udma_resources.desc, NULL,
				       (uint32_t *)&data->udma_handle);
	if (udma_handle != &data->udma_handle) {
		return -EINVAL;
	}

	/* Connect the DMA interrupt */
	cfg->irq_configure();

	if (UDMAx_DMAEnable(&udma_resources, udma_handle) != 0) {
		return -EBUSY;
	}

	return 0;
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
		if (data->zephyr_channel_info[channel].dma_callback) {
			/* Transfer complete, call user callback */
			data->zephyr_channel_info[channel].dma_callback(
				dev, data->zephyr_channel_info[channel].cb_data, channel, 0);
		}
		sys_write32(BIT(channel), (mem_addr_t)&cfg->reg->UDMA_DONE_STATUS_REG);
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
	SYS_MEM_BLOCKS_DEFINE_STATIC(desc_pool_##inst, sizeof(RSI_UDMA_DESC_T),                    \
				     CONFIG_DMA_SILABS_SIWX91X_SG_BUFFER_COUNT, 4);                \
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
	DEVICE_DT_INST_DEFINE(inst, &siwx91x_dma_init, NULL, &dma_data_##inst, &dma_cfg_##inst,    \
			      POST_KERNEL, CONFIG_DMA_INIT_PRIORITY, &siwx91x_dma_api);

DT_INST_FOREACH_STATUS_OKAY(SIWX91X_DMA_INIT)
