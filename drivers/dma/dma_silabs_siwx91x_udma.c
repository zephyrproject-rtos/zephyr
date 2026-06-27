/*
 * Copyright (c) 2024-2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/* This driver was originally based on upper layer of the Wiseconnect HAL (the
 * UDMAx_*() functions). This layer does not support Scatter-Gather transfers.
 * Implementation of Scatter-Gather has been implemented using direct access to
 * the hardware block. So, currently original implementation is used with only
 * one block is requested, otherwise SG implementation is used.
 *
 * SG implementation support single blocks operations. So, upper layer
 * implementation could probably be dropped. Then, hal_chans would be orphan and
 * hal_handle would also contains orphan fields.
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

#define SIWX91X_UDMA_MAX_XFER_COUNT      1024
#define SIWX91X_UDMA_MAX_PRIORITY           1
#define SIWX91X_UDMA_MIN_PRIORITY           0

/* This value is missing in rsi_udma.h */
#define UDMA_MODE_PER_ALT_SCATTER_GATHER 0x07

LOG_MODULE_REGISTER(siwx91x_udma, CONFIG_DMA_LOG_LEVEL);

struct siwx91x_udma_chan {
	dma_callback_t cb;
	void *cb_data;
	bool is_active;
	uint32_t dir;
	 /* Scatter-Gather table start address. Number of descriptors is stored
	  * in hal_chans[chan].Cnt
	  */
	RSI_UDMA_DESC_T *desc_base;
};

struct siwx91x_udma_config {
	UDMA0_Type *reg;
	/* An array of descriptor (one per channel). User may want to place this
	 * table in specific memory (ULP memory) to achieve specific power
	 * management.
	 * For single transfer, chan_desc[chan] directly store the memory
	 * information. the For SG transfers, it points to an array of
	 * RSI_UDMA_DESC_T.
	 */
	RSI_UDMA_DESC_T *chan_desc;
	uint8_t irq_number;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	void (*irq_configure)(void);
};

struct siwx91x_udma_data {
	struct dma_context dma_ctx;
	struct sys_mem_blocks *desc_pool;
	struct siwx91x_udma_chan *drv_chans;
	/* Equivalent of drv_chans for upper layer API of the HAL */
	UDMA_Channel_Info *hal_chans;
	RSI_UDMA_DATACONTEXT_T hal_handle;
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

static int siwx91x_udma_fill_block_descs(const struct dma_config *dma_cfg,
					 const struct dma_block_config *block,
					 RSI_UDMA_DESC_T *descs)
{
	uint8_t *src_addr = (uint8_t *)block->source_address;
	uint8_t *dst_addr = (uint8_t *)block->dest_address;
	uint32_t remaining = block->block_size;
	int cnt = 0;

	do {
		uint32_t chunk = SIWX91X_UDMA_MAX_XFER_COUNT * dma_cfg->source_burst_length;
		RSI_UDMA_CHA_CONFIG_DATA_T *desc_cfg =
			(RSI_UDMA_CHA_CONFIG_DATA_T *)&descs->vsUDMAChaConfigData1;

		if (chunk > remaining) {
			chunk = remaining;
		}
		desc_cfg->srcSize = find_lsb_set(dma_cfg->source_burst_length) - 1;
		desc_cfg->dstSize = find_lsb_set(dma_cfg->dest_burst_length) - 1;
		desc_cfg->totalNumOfDMATrans = (chunk / dma_cfg->source_burst_length) - 1;

		if (block->source_addr_adj == DMA_ADDR_ADJ_NO_CHANGE) {
			descs->pSrcEndAddr = src_addr;
		} else {
			descs->pSrcEndAddr = src_addr + chunk - dma_cfg->source_burst_length;
		}

		if (block->dest_addr_adj == DMA_ADDR_ADJ_NO_CHANGE) {
			descs->pDstEndAddr = dst_addr;
		} else {
			descs->pDstEndAddr = dst_addr + chunk - dma_cfg->dest_burst_length;
		}

		/* Set the transfer type based on whether it is a peripheral request */
		if (dma_cfg->channel_direction == MEMORY_TO_MEMORY) {
			desc_cfg->transferType = UDMA_MODE_MEM_ALT_SCATTER_GATHER;
		} else {
			desc_cfg->transferType = UDMA_MODE_PER_ALT_SCATTER_GATHER;
		}

		desc_cfg->rPower = ARBSIZE_1;

		if (block->source_addr_adj == DMA_ADDR_ADJ_NO_CHANGE) {
			desc_cfg->srcInc = UDMA_SRC_INC_NONE;
		} else {
			desc_cfg->srcInc = find_lsb_set(dma_cfg->source_burst_length) - 1;
		}

		if (block->dest_addr_adj == DMA_ADDR_ADJ_NO_CHANGE) {
			desc_cfg->dstInc = UDMA_DST_INC_NONE;
		} else {
			desc_cfg->dstInc = find_lsb_set(dma_cfg->dest_burst_length) - 1;
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

static int siwx91x_udma_fill_sg_descs(const struct dma_config *dma_cfg,
				      RSI_UDMA_DESC_T *desc_base, uint32_t desc_count)
{
	const struct dma_block_config *block = dma_cfg->head_block;
	int cnt = 0;

	for (int i = 0; i < dma_cfg->block_count; i++) {
		if (!siwx91x_udma_is_valid_adjustment(block->source_addr_adj) ||
		    !siwx91x_udma_is_valid_adjustment(block->dest_addr_adj)) {
			return -EINVAL;
		}
		if (block->block_size % dma_cfg->source_burst_length) {
			return -EINVAL;
		}

		cnt += siwx91x_udma_fill_block_descs(dma_cfg, block, desc_base + cnt);
		block = block->next_block;
	}
	__ASSERT(cnt == desc_count, "Corrupted state");

	if (block != NULL) {
		return -EINVAL;
	}

	/* Set the transfer type for the last descriptor */
	if (dma_cfg->channel_direction == MEMORY_TO_MEMORY) {
		desc_base[cnt - 1].vsUDMAChaConfigData1.transferType = UDMA_MODE_AUTO;
	} else {
		desc_base[cnt - 1].vsUDMAChaConfigData1.transferType = UDMA_MODE_BASIC;
	}

	return 0;
}

static uint32_t siwx91x_udma_count_hw_desc(const struct dma_config *dma_cfg)
{
	const struct dma_block_config *block = dma_cfg->head_block;
	uint32_t max_chunk = SIWX91X_UDMA_MAX_XFER_COUNT * dma_cfg->source_burst_length;
	uint32_t total = 0;

	for (int i = 0; i < dma_cfg->block_count; i++) {
		if (block->block_size) {
			total += DIV_ROUND_UP(block->block_size, max_chunk);
		} else {
			total += 1;
		}
		block = block->next_block;
	}

	return total;
}

static int siwx91x_udma_config_sg(const struct device *dev, uint32_t channel,
				  const struct dma_config *dma_cfg)
{
	const struct siwx91x_udma_config *cfg = dev->config;
	struct siwx91x_udma_data *data = dev->data;
	uint32_t desc_count = siwx91x_udma_count_hw_desc(dma_cfg);
	RSI_UDMA_DESC_T *desc_base = NULL;
	uint8_t transfer_type;
	int ret;

	ret = sys_mem_blocks_alloc_contiguous(data->desc_pool, desc_count,
					    (void **)&desc_base);
	if (ret) {
		return -EINVAL;
	}

	ret = siwx91x_udma_fill_sg_descs(dma_cfg, desc_base, desc_count);
	if (ret) {
		sys_mem_blocks_free_contiguous(data->desc_pool, (void *)desc_base, desc_count);
		return -EINVAL;
	}

	/* This channel information is used to distinguish scatter-gather transfers and
	 * free the allocated descriptors in cfg->chan_desc
	 */
	data->hal_chans[channel].Cnt = desc_count;
	data->drv_chans[channel].desc_base = desc_base;

	/* Store the transfer direction. This is used to trigger SW request for
	 * Memory to Memory transfers.
	 */
	data->drv_chans[channel].dir = dma_cfg->channel_direction;

	RSI_UDMA_InterruptClear(&data->hal_handle, channel);
	RSI_UDMA_ErrorStatusClear(&data->hal_handle);

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

	if (dma_cfg->channel_direction == MEMORY_TO_MEMORY) {
		transfer_type = UDMA_MODE_MEM_SCATTER_GATHER;
	} else {
		transfer_type = UDMA_MODE_PER_SCATTER_GATHER;
	}
	RSI_UDMA_SetChannelScatterGatherTransfer(&data->hal_handle, channel, desc_count,
						 desc_base, transfer_type);
	return 0;
}

static int siwx91x_udma_config_single(const struct device *dev, uint32_t channel,
				      const struct dma_config *dma_cfg)
{
	uint32_t dma_transfer_num = dma_cfg->head_block->block_size / dma_cfg->source_burst_length;
	const struct siwx91x_udma_config *cfg = dev->config;
	struct siwx91x_udma_data *data = dev->data;
	UDMA_RESOURCES udma_resources = {
		.reg = cfg->reg,
		.udma_irq_num = cfg->irq_number,
		/* SRAM address where UDMA descriptor is stored */
		.desc = cfg->chan_desc,
	};
	RSI_UDMA_CHA_CONFIG_DATA_T channel_control = {
		.transferType = UDMA_MODE_BASIC,
	};
	RSI_UDMA_CHA_CFG_T channel_config = {};
	int status;

	channel_config.channelPrioHigh = dma_cfg->channel_priority;
	channel_config.dmaCh = channel;

	if (dma_cfg->channel_direction == MEMORY_TO_MEMORY) {
		channel_control.rPower = ARBSIZE_1024;
		channel_config.periphReq = 0;
	} else {
		channel_control.rPower = ARBSIZE_1;
		channel_config.periphReq = 1;
	}

	/* Obtain the number of transfers */
	if (dma_transfer_num >= SIWX91X_UDMA_MAX_XFER_COUNT) {
		/* Maximum number of transfers is 1024 */
		channel_control.totalNumOfDMATrans = SIWX91X_UDMA_MAX_XFER_COUNT - 1;
	} else {
		channel_control.totalNumOfDMATrans = dma_transfer_num;
	}

	channel_control.srcSize = find_lsb_set(dma_cfg->source_burst_length) - 1;
	channel_control.dstSize = find_lsb_set(dma_cfg->dest_burst_length) - 1;

	if (!siwx91x_udma_is_valid_adjustment(dma_cfg->head_block->source_addr_adj) ||
	    !siwx91x_udma_is_valid_adjustment(dma_cfg->head_block->dest_addr_adj)) {
		return -EINVAL;
	}

	if (dma_cfg->head_block->source_addr_adj == DMA_ADDR_ADJ_NO_CHANGE) {
		channel_control.srcInc = UDMA_SRC_INC_NONE;
	} else {
		channel_control.srcInc = find_lsb_set(dma_cfg->source_burst_length) - 1;
	}

	if (dma_cfg->head_block->dest_addr_adj == DMA_ADDR_ADJ_NO_CHANGE) {
		channel_control.dstInc = UDMA_DST_INC_NONE;
	} else {
		channel_control.dstInc = find_lsb_set(dma_cfg->dest_burst_length) - 1;
	}

	/* Clear the CHNL_PRI_ALT_CLR to use primary DMA descriptor structure */
	sys_write32(BIT(channel), (mem_addr_t)&cfg->reg->CHNL_PRI_ALT_CLR);

	status = UDMAx_ChannelConfigure(&udma_resources, (uint8_t)channel,
					dma_cfg->head_block->source_address,
					dma_cfg->head_block->dest_address,
					dma_transfer_num, channel_control,
					&channel_config, NULL, data->hal_chans,
					&data->hal_handle);
	if (status) {
		return -EIO;
	}

	/* Store the transfer direction. This is used to trigger SW request for
	 * Memory to Memory transfers.
	 */
	data->drv_chans[channel].dir = dma_cfg->channel_direction;

	return 0;
}

static int siwx91x_udma_config(const struct device *dev, uint32_t channel,
			       struct dma_config *dma_cfg)
{
	struct siwx91x_udma_data *data = dev->data;
	int status;

	/* Expecting a fixed channel number between 0-31 for dma0 and 0-11 for ulpdma */
	if (channel >= data->dma_ctx.dma_channels) {
		return -EINVAL;
	}

	/* Disable the channel before configuring */
	if (RSI_UDMA_ChannelDisable(&data->hal_handle, channel) != 0) {
		return -EIO;
	}

	if (dma_cfg->channel_priority < SIWX91X_UDMA_MIN_PRIORITY ||
	    dma_cfg->channel_priority > SIWX91X_UDMA_MAX_PRIORITY) {
		return -EINVAL;
	}

	if (dma_cfg->cyclic || dma_cfg->complete_callback_en) {
		return -EINVAL;
	}

	if (!siwx91x_udma_is_valid_dir(dma_cfg->channel_direction)) {
		return -EINVAL;
	}

	if (!siwx91x_udma_is_valid_width(dma_cfg->source_data_size) ||
	    !siwx91x_udma_is_valid_width(dma_cfg->dest_data_size)) {
		return -EINVAL;
	}

	if (!siwx91x_udma_is_valid_burst_len(dma_cfg->source_burst_length) ||
	    !siwx91x_udma_is_valid_burst_len(dma_cfg->dest_burst_length)) {
		return -EINVAL;
	}

	if (dma_cfg->head_block->next_block != NULL) {
		status = siwx91x_udma_config_sg(dev, channel, dma_cfg);
	} else {
		status = siwx91x_udma_config_single(dev, channel, dma_cfg);
	}
	if (status) {
		return status;
	}

	data->drv_chans[channel].cb = dma_cfg->dma_callback;
	data->drv_chans[channel].cb_data = dma_cfg->user_data;

	atomic_set_bit(data->dma_ctx.atomic, channel);

	return 0;
}

static int siwx91x_udma_reload(const struct device *dev, uint32_t channel, uint32_t src,
			       uint32_t dst, size_t size)
{
	const struct siwx91x_udma_config *cfg = dev->config;
	struct siwx91x_udma_data *data = dev->data;
	RSI_UDMA_DESC_T *chan_desc = &cfg->chan_desc[channel];
	uint32_t length;

	/* Expecting a fixed channel number between 0-31 for dma0 and 0-11 for ulpdma */
	if (channel >= data->dma_ctx.dma_channels) {
		return -EINVAL;
	}

	/* SG transfers can't be reloaded */
	if (data->drv_chans[channel].desc_base) {
		return -EINVAL;
	}

	if (size % BIT(chan_desc->vsUDMAChaConfigData1.srcSize)) {
		return -EINVAL;
	}

	if (RSI_UDMA_ChannelDisable(&data->hal_handle, channel)) {
		return -EIO;
	}

	/* Update new channel info to dev->data structure */
	data->hal_chans[channel].SrcAddr = src;
	data->hal_chans[channel].DestAddr = dst;
	data->hal_chans[channel].Size = size >> chan_desc->vsUDMAChaConfigData1.srcSize;

	/* Update new transfer size to dev->data structure */
	if (data->hal_chans[channel].Size >= SIWX91X_UDMA_MAX_XFER_COUNT) {
		data->hal_chans[channel].Cnt = SIWX91X_UDMA_MAX_XFER_COUNT - 1;
	} else {
		data->hal_chans[channel].Cnt = size >> chan_desc->vsUDMAChaConfigData1.srcSize;
	}

	/* Program the DMA descriptors with new transfer data information. */
	if (chan_desc->vsUDMAChaConfigData1.srcInc != UDMA_SRC_INC_NONE) {
		length = data->hal_chans[channel].Cnt << chan_desc->vsUDMAChaConfigData1.srcInc;
		chan_desc->pSrcEndAddr = (void *)(src + length - 1);
	}

	if (chan_desc->vsUDMAChaConfigData1.dstInc != UDMA_SRC_INC_NONE) {
		length = data->hal_chans[channel].Cnt << chan_desc->vsUDMAChaConfigData1.dstInc;
		chan_desc->pDstEndAddr = (void *)(dst + length - 1);
	}

	chan_desc->vsUDMAChaConfigData1.totalNumOfDMATrans = data->hal_chans[channel].Cnt - 1;
	chan_desc->vsUDMAChaConfigData1.transferType = UDMA_MODE_BASIC;

	return 0;
}

static int siwx91x_udma_start(const struct device *dev, uint32_t channel)
{
	const struct siwx91x_udma_config *cfg = dev->config;
	struct siwx91x_udma_data *data = dev->data;

	/* Expecting a fixed channel number between 0-31 for dma0 and 0-11 for ulpdma */
	if (channel >= data->dma_ctx.dma_channels) {
		return -EINVAL;
	}

	if (!data->drv_chans[channel].is_active) {
		pm_device_runtime_get(dev);
		data->drv_chans[channel].is_active = true;
	}

	if (RSI_UDMA_ChannelEnable(&data->hal_handle, channel) != 0) {
		if (data->drv_chans[channel].is_active) {
			pm_device_runtime_put(dev);
			data->drv_chans[channel].is_active = false;
		}
		return -EINVAL;
	}

	/* Check if the transfer type is memory-memory */
	if (data->drv_chans[channel].dir == MEMORY_TO_MEMORY) {
		/* Apply software trigger to start transfer */
		sys_set_bit((mem_addr_t)&cfg->reg->CHNL_SW_REQUEST, channel);
	}

	return 0;
}

static int siwx91x_udma_stop(const struct device *dev, uint32_t channel)
{
	struct siwx91x_udma_data *data = dev->data;

	/* Expecting a fixed channel number between 0-31 for dma0 and 0-11 for ulpdma */
	if (channel >= data->dma_ctx.dma_channels) {
		return -EINVAL;
	}

	if (RSI_UDMA_ChannelDisable(&data->hal_handle, channel) != 0) {
		return -EIO;
	}

	if (data->drv_chans[channel].is_active) {
		pm_device_runtime_put(dev);
		data->drv_chans[channel].is_active = false;
	}

	return 0;
}

static int siwx91x_udma_get_status(const struct device *dev, uint32_t channel,
				   struct dma_status *stat)
{
	const struct siwx91x_udma_config *cfg = dev->config;
	struct siwx91x_udma_data *data = dev->data;

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
	if (cfg->chan_desc[channel].vsUDMAChaConfigData1.srcInc == UDMA_SRC_INC_NONE) {
		stat->dir = PERIPHERAL_TO_MEMORY;
	} else if (cfg->chan_desc[channel].vsUDMAChaConfigData1.dstInc == UDMA_DST_INC_NONE) {
		stat->dir = MEMORY_TO_PERIPHERAL;
	} else {
		stat->dir = MEMORY_TO_MEMORY;
	}

	return 0;
}

bool siwx91x_udma_chan_filter(const struct device *dev, int channel, void *filter_param)
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

static int siwx91x_udma_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct siwx91x_udma_config *cfg = dev->config;
	struct siwx91x_udma_data *data = dev->data;
	void *udma_handle = NULL;
	UDMA_RESOURCES udma_resources = {
		.reg = cfg->reg, /* UDMA register base address */
		.udma_irq_num = cfg->irq_number,
		.desc = cfg->chan_desc,
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
					       (uint32_t *)&data->hal_handle);
		if (udma_handle != &data->hal_handle) {
			return -EINVAL;
		}

		if (UDMAx_DMAEnable(&udma_resources, &data->hal_handle) != 0) {
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
static int siwx91x_udma_init(const struct device *dev)
{
	const struct siwx91x_udma_config *cfg = dev->config;

	/* Connect the DMA interrupt */
	cfg->irq_configure();

	return pm_device_driver_init(dev, siwx91x_udma_pm_action);
}

static void siwx91x_udma_isr(const struct device *dev)
{
	const struct siwx91x_udma_config *cfg = dev->config;
	struct siwx91x_udma_data *data = dev->data;
	UDMA_RESOURCES udma_resources = {
		.reg = cfg->reg,
		.udma_irq_num = cfg->irq_number,
		.desc = cfg->chan_desc,
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

	if (data->drv_chans[channel].desc_base) {
		/* A Scatter-Gather transfer is completed, free the allocated descriptors */
		if (sys_mem_blocks_free_contiguous(data->desc_pool,
						   (void *)data->drv_chans[channel].desc_base,
						   data->hal_chans[channel].Cnt)) {
			sys_write32(BIT(channel), (mem_addr_t)&cfg->reg->UDMA_DONE_STATUS_REG);
			goto out;
		}
		data->hal_chans[channel].Cnt = 0;
		data->hal_chans[channel].Size = 0;
		data->drv_chans[channel].desc_base = NULL;
	}

	if (data->hal_chans[channel].Cnt == data->hal_chans[channel].Size) {
		sys_write32(BIT(channel), (mem_addr_t)&cfg->reg->UDMA_DONE_STATUS_REG);
		if (data->drv_chans[channel].is_active) {
			pm_device_runtime_put_async(dev, K_NO_WAIT);
			data->drv_chans[channel].is_active = false;
		}
		if (data->drv_chans[channel].cb) {
			data->drv_chans[channel].cb(dev, data->drv_chans[channel].cb_data,
						    channel, 0);
		}
	} else {
		/* Call UDMA ROM IRQ handler. */
		ROMAPI_UDMA_WRAPPER_API->uDMAx_IRQHandler(&udma_resources, udma_resources.desc,
							  data->hal_chans);
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
static DEVICE_API(dma, siwx91x_udma_api) = {
	.config = siwx91x_udma_config,
	.reload = siwx91x_udma_reload,
	.start = siwx91x_udma_start,
	.stop = siwx91x_udma_stop,
	.get_status = siwx91x_udma_get_status,
	.chan_filter = siwx91x_udma_chan_filter,
};

#define SIWX91X_UDMA_CHAN_DESC_ADDR(inst)                                                          \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, silabs_sram_region),                               \
		    ((RSI_UDMA_DESC_T *)DT_REG_ADDR(DT_INST_PHANDLE(inst, silabs_sram_region))),   \
		    (siwx91x_udma_chan_desc##inst))                                                \

#define SIWX91X_UDMA_INIT(inst)                                                                    \
	static ATOMIC_DEFINE(siwx91x_udma_chans_atomic##inst, DT_INST_PROP(inst, dma_channels));   \
	static UDMA_Channel_Info siwx91x_udma_hal_chans##inst[DT_INST_PROP(inst, dma_channels)];   \
	SYS_MEM_BLOCKS_DEFINE_STATIC_TYPE(siwx91x_udma_desc_pool##inst, RSI_UDMA_DESC_T,           \
					  CONFIG_DMA_SILABS_SIWX91X_UDMA_DESCR_COUNT);             \
	IF_DISABLED(DT_INST_NODE_HAS_PROP(inst, silabs_sram_region),                               \
		    (static __aligned(1024) RSI_UDMA_DESC_T                                        \
			     siwx91x_udma_chan_desc##inst[DT_INST_PROP(inst, dma_channels) * 2];)) \
	static struct siwx91x_udma_chan                                                            \
		siwx91x_udma_drv_chans##inst[DT_INST_PROP(inst, dma_channels)];                    \
	static struct siwx91x_udma_data siwx91x_udma_data##inst = {                                \
		.dma_ctx.magic = DMA_MAGIC,                                                        \
		.dma_ctx.dma_channels = DT_INST_PROP(inst, dma_channels),                          \
		.dma_ctx.atomic = siwx91x_udma_chans_atomic##inst,                                 \
		.hal_chans = siwx91x_udma_hal_chans##inst,                                         \
		.drv_chans = siwx91x_udma_drv_chans##inst,                                         \
		.desc_pool = &siwx91x_udma_desc_pool##inst,                                        \
	};                                                                                         \
	static void siwx91x_udma_irq_configure##inst(void)                                         \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ(inst, irq), DT_INST_IRQ(inst, priority),                   \
			    siwx91x_udma_isr, DEVICE_DT_INST_GET(inst), 0);                        \
		irq_enable(DT_INST_IRQ(inst, irq));                                                \
	}                                                                                          \
	static const struct siwx91x_udma_config siwx91x_udma_cfg##inst = {                         \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                             \
		.clock_subsys = (clock_control_subsys_t)DT_INST_PHA(inst, clocks, clkid),          \
		.reg = (UDMA0_Type *)DT_INST_REG_ADDR(inst),                                       \
		.irq_number = DT_INST_PROP_BY_IDX(inst, interrupts, 0),                            \
		.chan_desc = SIWX91X_UDMA_CHAN_DESC_ADDR(inst),                                    \
		.irq_configure = siwx91x_udma_irq_configure##inst,                                 \
	};                                                                                         \
	PM_DEVICE_DT_INST_DEFINE(inst, siwx91x_udma_pm_action);                                    \
	DEVICE_DT_INST_DEFINE(inst, siwx91x_udma_init,  PM_DEVICE_DT_INST_GET(inst),               \
			      &siwx91x_udma_data##inst, &siwx91x_udma_cfg##inst, POST_KERNEL,      \
			      CONFIG_DMA_INIT_PRIORITY, &siwx91x_udma_api);

DT_INST_FOREACH_STATUS_OKAY(SIWX91X_UDMA_INIT)
