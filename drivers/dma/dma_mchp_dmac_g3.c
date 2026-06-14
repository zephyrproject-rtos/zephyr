/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <soc.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mchp_clock_control.h>
#include <zephyr/kernel.h>

#define DT_DRV_COMPAT microchip_dmac_g3_dma

LOG_MODULE_REGISTER(dma_mchp_dmac_g3, CONFIG_DMA_LOG_LEVEL);

#define DMAC_BUF_ADDR_ALIGNMENT 1
#define DMAC_BUF_SIZE_ALIGNMENT 1
#define DMAC_COPY_ALIGNMENT     1
#define DMAC_MAX_BLOCK_COUNT    65535
#define DMAC_NULL               (0xFFFFFFFF)
#define DMAC_NULLP              ((dma_descriptor_registers_t *)(0xFFFFFFFF))
#define DMAC_TIMEOUT_VALUE_US   1000

#define DMAC_NIBS_BIT_COUNT 4
#define DMAC_CH_NIBS        4
#define DMAC_NIBS_MASK      0xF
#define DMAC_TRIG_MAX       90

/* A descriptor must reside within a 1KB address boundary. The DMA may perform speculative reads
 * on the descriptor content; therefore the first word of the descriptor must reside a minimum of
 * ten words below a memory boundary to avoid speculative reads into unmapped memory regions.
 */
#define DMAC_DESC_SIZE      40
#define DMAC_DESC_ALIGN_LEN 4
#define DMAC_DESC_PER_1KB   (1024 / DMAC_DESC_SIZE)
#define DMAC_DESC_MEM_COUNT                                                                        \
	(1 + CONFIG_DMAC_DESC_COUNT + (CONFIG_DMAC_DESC_COUNT / DMAC_DESC_PER_1KB))

enum dma_mchp_attribute_type {
	DMA_MCHP_ATTR_BUFFER_ADDRESS_ALIGNMENT,
	DMA_MCHP_ATTR_BUFFER_SIZE_ALIGNMENT,
	DMA_MCHP_ATTR_COPY_ALIGNMENT,
	DMA_MCHP_ATTR_MAX_BLOCK_COUNT,
};

struct dma_mchp_channel_config {
	dma_callback_t cb;
	void *user_data;
	dma_descriptor_registers_t *desc_head;
	bool is_configured;
	bool is_cyclic;
};

struct dma_mchp_dev_config {
	dma_registers_t *regs;

	/* Pointer to the clock device for DMAC. */
	const struct device *clock_dev;

	/* Clock control subsystem for DMAC. */
	clock_control_subsys_t mclk_sys;

	/* Function pointer for configuring DMA interrupts. */
	void (*irq_config)(void);
};

static __aligned(DMAC_DESC_ALIGN_LEN) dma_descriptor_registers_t dmac_desc_arr[DMAC_DESC_MEM_COUNT];

struct dma_mchp_dev_data {
	struct dma_context dma_ctx;
	struct dma_mchp_channel_config *dma_channel_config;
	dma_descriptor_registers_t *dmac_desc_pool;
};

/* Initialize descriptor pool as a linked list */
static void dmac_desc_pool_init(const struct device *dev)
{
	int i = 0, allocated = 0;
	struct dma_mchp_dev_data *const dev_data = ((struct dma_mchp_dev_data *const)(dev)->data);

	/* First word of descriptor must reside in a minimum of ten words after a 1KB
	 * memory boundary to allow speculative reads. Skip if not.
	 */
	if (((uint32_t)(&dmac_desc_arr[i]) & 0x3FF) < DMAC_DESC_SIZE) {
		i++;
	}
	dev_data->dmac_desc_pool = &dmac_desc_arr[i];
	allocated = i;

	/* Link all descriptors in array */
	while (i < (DMAC_DESC_MEM_COUNT - 1)) {
		i++;
		if (((uint32_t)(&dmac_desc_arr[i]) & 0x3FF) < DMAC_DESC_SIZE) {
			continue;
		}
		dmac_desc_arr[allocated].DMA_BDNXT = (uint32_t)(&dmac_desc_arr[i]);
		allocated = i;
	}
	dmac_desc_arr[allocated].DMA_BDNXT = DMAC_NULL;
}

/* Get a descriptor from pool */
static inline dma_descriptor_registers_t *dmac_desc_getfrom_pool(const struct device *dev)
{
	struct dma_mchp_dev_data *const dev_data = ((struct dma_mchp_dev_data *const)(dev)->data);
	dma_descriptor_registers_t *ret_desc;
	unsigned int key;

	key = irq_lock();
	ret_desc = dev_data->dmac_desc_pool;
	if (ret_desc != DMAC_NULLP) {
		dev_data->dmac_desc_pool = (dma_descriptor_registers_t *)(ret_desc->DMA_BDNXT);
	}

	irq_unlock(key);

	return ret_desc;
}

/* Free one descriptor to the pool */
static inline void dmac_desc_freeto_pool(const struct device *dev, dma_descriptor_registers_t *desc)
{
	struct dma_mchp_dev_data *const dev_data = ((struct dma_mchp_dev_data *const)(dev)->data);
	unsigned int key;

	key = irq_lock();
	desc->DMA_BDNXT = (uint32_t)(dev_data->dmac_desc_pool);
	dev_data->dmac_desc_pool = desc;

	irq_unlock(key);
}

/* Free the descriptors allocated for this channel */
void dmac_desc_freeall(const struct device *dev,
		       struct dma_mchp_channel_config *channel_config_data)
{
	dma_descriptor_registers_t *desc, *desc_head;

	desc_head = channel_config_data->desc_head;
	while ((desc_head != DMAC_NULLP) && (desc_head != NULL)) {
		desc = (dma_descriptor_registers_t *)(desc_head->DMA_BDNXT);
		dmac_desc_freeto_pool(dev, desc_head);
		desc_head = desc;
		if (desc_head == channel_config_data->desc_head) {
			/* Cyclic */
			break;
		}
	}
	channel_config_data->desc_head = DMAC_NULLP;
}

/* Check if dmac channel is busy */
static bool dmac_ch_is_busy(dma_channel_registers_t *channel_regs)
{
	if (((channel_regs->DMA_CHSTAT & (DMA_CHSTAT_CELLBUSY_Msk | DMA_CHSTAT_BLKBUSY_Msk)) ==
	     0) &&
	    ((channel_regs->DMA_CHCTRLA & DMA_CHCTRLA_ENABLE_Msk) == 0)) {
		return false;
	} else {
		return true;
	}
}

/* Configure a descriptor */
static int dmac_setup_desc(struct dma_block_config *desc_config, dma_descriptor_registers_t *desc,
			   const struct dma_config *config)
{
	uint32_t burst_len, ras, was, prio, auto_start, val32;

	if (config->channel_priority > DMA_BDCTRLB_PRI_PRI_4_Val) {
		LOG_ERR("Invalid channel priority");
		return -EINVAL;
	}
	prio = DMA_BDCTRLB_PRI(config->channel_priority);

	/*  Read Address Sequence Mode */
	if (desc_config->source_addr_adj == DMA_ADDR_ADJ_INCREMENT) {
		ras = DMA_BDCTRLB_RAS_AUTO_ADDR_INCR;
	} else if (desc_config->source_addr_adj == DMA_ADDR_ADJ_NO_CHANGE) {
		switch (config->source_data_size) {
		case 1:
			ras = DMA_BDCTRLB_RAS_FIXED_BYTE_ADDR_INCR;
			break;
		case 2:
			ras = DMA_BDCTRLB_RAS_FIXED_HALF_WORD_ADDR_INCR;
			break;
		case 4:
			ras = DMA_BDCTRLB_RAS_FIXED_WORD_ADDR_INCR;
			break;
		default:
			LOG_ERR("Invalid parameter for DMA source data size");
			return -EINVAL;
		}
	} else {
		LOG_ERR("Invalid parameter for DMA source address adj");
		return -EINVAL;
	}

	/* Write Address Sequence Mode */
	if (desc_config->dest_addr_adj == DMA_ADDR_ADJ_INCREMENT) {
		was = DMA_BDCTRLB_WAS_AUTO_ADDR_INCR;
	} else if (desc_config->dest_addr_adj == DMA_ADDR_ADJ_NO_CHANGE) {
		switch (config->dest_data_size) {
		case 1:
			was = DMA_BDCTRLB_WAS_FIXED_BYTE_ADDR_INCR;
			break;
		case 2:
			was = DMA_BDCTRLB_WAS_FIXED_HALF_WORD_ADDR_INCR;
			break;
		case 4:
			was = DMA_BDCTRLB_WAS_FIXED_WORD_ADDR_INCR;
			break;
		default:
			LOG_ERR("Invalid parameter for DMA destination data size");
			return -EINVAL;
		}
	} else {
		LOG_ERR("Invalid parameter for DMA destination address adj");
		return -EINVAL;
	}

	auto_start = (config->channel_direction == MEMORY_TO_MEMORY) ? DMA_BDCTRLB_CASTEN(1)
								     : DMA_BDCTRLB_CASTEN(0);

	val32 = ras | was | prio | auto_start;

	/* If programmed to a value greater than the maximum trigger index, or 0, all
	 * external triggers are disabled. Only software triggering is available.
	 */
	if (config->dma_slot <= DMAC_TRIG_MAX) {
		val32 |= DMA_BDCTRLB_TRIG(config->dma_slot);
	}
	desc->DMA_BDCTRLB = val32;

	/* Descriptor source and destination start address */
	desc->DMA_BDSSA = desc_config->source_address;
	desc->DMA_BDDSA = desc_config->dest_address;

	/* Device support cell transfer size per trigger. If peripheral burstlen specified
	 * in API, we use the same. Else, we use the smaller of source/destination burstlen.
	 */
	if (config->channel_direction == MEMORY_TO_PERIPHERAL) {
		burst_len = config->dest_burst_length;
	} else if (config->channel_direction == PERIPHERAL_TO_MEMORY) {
		burst_len = config->source_burst_length;
	} else {
		burst_len = (config->dest_burst_length < config->source_burst_length)
				    ? config->dest_burst_length
				    : config->source_burst_length;
	}
	if (burst_len > (DMA_CHXSIZ_CSZ_Msk >> DMA_CHXSIZ_CSZ_Pos)) {
		return -EINVAL;
	}

	/* Block and Cell Transfer Size */
	desc->DMA_BDXSIZ = DMA_BDXSIZ_BLKSZ(desc_config->block_size) | DMA_BDXSIZ_CSZ(burst_len);

	/* Source and Destination Cell Stride Size */
	desc->DMA_BDSSTRD = 0;
	desc->DMA_BDDSTRD = 0;

	/* Enable the DMA channel,  and start the transfer */
	desc->DMA_BDCFG = DMA_BDCFG_CTRLB_Msk | DMA_BDCFG_SSA_Msk | DMA_BDCFG_DSA_Msk |
			  DMA_BDCFG_XSIZ_Msk | DMA_BDCFG_ENABLE_Msk | DMA_BDCFG_LLEN_Msk;
	if ((DMA_BDCTRLB_TRIG_Msk & desc->DMA_BDCTRLB) == 0) {
		/* Trigger via software */
		desc->DMA_BDCFG |= DMA_BDCFG_SWFRC_Msk;
	}

	return 0;
}

/* Configure a channel */
static int dmac_setup_channel(const struct device *dev, uint32_t channel,
			      const struct dma_config *config,
			      dma_descriptor_registers_t *desc_list)
{
	struct dma_mchp_dev_data *const dev_data = dev->data;
	dma_registers_t *dmac_reg = ((const struct dma_mchp_dev_config *)(dev)->config)->regs;
	dma_channel_registers_t *channel_regs = &(dmac_reg->CHANNEL[channel]);
	struct dma_block_config *desc_config = config->head_block;
	struct dma_mchp_channel_config *channel_config_data =
		&dev_data->dma_channel_config[channel];
	uint32_t block_count = config->block_count;

	/* Start using the channel */
	atomic_test_and_set_bit(dev_data->dma_ctx.atomic, channel);
	channel_regs->DMA_CHNXT = (uint32_t)desc_list;
	channel_config_data->desc_head = desc_list;
	channel_config_data->is_cyclic = (config->cyclic != 0);

	/* Configure each descriptor and Traverse to next block */
	while (block_count != 0) {
		block_count--;
		if (dmac_setup_desc(desc_config, desc_list, config) != 0) {
			return -EINVAL;
		}
		if ((config->cyclic != 0) && (block_count == 0)) {
			desc_list->DMA_BDNXT = (uint32_t)(channel_config_data->desc_head);
		}
		desc_config = desc_config->next_block;
		desc_list = (dma_descriptor_registers_t *)(desc_list->DMA_BDNXT);
	}

	/* Clear any pending interrupt flags */
	channel_regs->DMA_CHINTENCLR = DMA_CHINTENCLR_Msk;
	channel_regs->DMA_CHINTF = DMA_CHINTF_Msk;

	/* Enable transfer complete interrupt */
	channel_regs->DMA_CHINTENSET = (config->complete_callback_en == true)
					       ? DMA_CHINTENSET_BC_Msk
					       : DMA_CHINTENSET_LL_Msk;

	channel_config_data->cb = config->dma_callback;
	channel_config_data->user_data = config->user_data;
	channel_config_data->is_configured = true;

	return 0;
}

/* Initialize and configure a specified DMA channel */
static int dma_mchp_config(const struct device *dev, uint32_t channel, struct dma_config *config)
{
	struct dma_mchp_dev_data *const dev_data = dev->data;
	dma_registers_t *dmac_reg = ((const struct dma_mchp_dev_config *)(dev)->config)->regs;
	dma_channel_registers_t *channel_regs = &(dmac_reg->CHANNEL[channel]);
	dma_descriptor_registers_t *desc, *desc_list;
	struct dma_block_config *desc_config;
	uint32_t block_count;
	struct dma_mchp_channel_config *channel_config_data;

	if (channel >= dev_data->dma_ctx.dma_channels) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	/* Check for valid block */
	if (config->head_block == NULL) {
		LOG_ERR("head_block is NULL in config");
		return -EINVAL;
	}

	if ((config->source_chaining_en == true) || (config->dest_chaining_en == true)) {
		LOG_ERR("chaining not supported");
		return -ENOTSUP;
	}

	/* Check if the channel is already in use */
	if (dmac_ch_is_busy(channel_regs) == true) {
		LOG_ERR("DMA channel %d is already in use", channel);
		return -EBUSY;
	}

	/* Free the descriptors allocated for this channel before. */
	channel_config_data = &dev_data->dma_channel_config[channel];
	dmac_desc_freeall(dev, channel_config_data);

	/* Pre-allocate enough descriptors from pool. If not available, return error.
	 * This is done upfront, since the pool is shared,
	 * and we need to have all descriptors upfront before we can proceed.
	 */
	desc_config = config->head_block;
	block_count = config->block_count;
	desc_list = DMAC_NULLP;
	while ((desc_config != NULL) && (block_count != 0)) {
		desc = dmac_desc_getfrom_pool(dev);
		if (desc == DMAC_NULLP) {
			/* free the preallocated descriptors back, one by one */
			while (desc_list != DMAC_NULLP) {
				desc = desc_list;
				desc_list = (dma_descriptor_registers_t *)(desc_list->DMA_BDNXT);
				dmac_desc_freeto_pool(dev, desc);
			}
			LOG_ERR("No more descriptors to configure");
			return -ENOMEM;
		}
		desc_config = desc_config->next_block;
		block_count--;

		/* Add allocated descriptor to list for use in this function */
		desc->DMA_BDNXT = (uint32_t)desc_list;
		desc_list = desc;
	}
	if (block_count != 0) {
		LOG_ERR("block_count not matching");
		return -EINVAL;
	}

	if (dmac_setup_channel(dev, channel, config, desc_list) != 0) {
		return -EINVAL;
	}

	return 0;
}

/* Start a dma transfer */
static int dma_mchp_start(const struct device *dev, uint32_t channel)
{
	/* Get dev data */
	struct dma_mchp_dev_data *const dev_data = dev->data;
	dma_registers_t *dmac_reg = ((const struct dma_mchp_dev_config *)(dev)->config)->regs;
	dma_channel_registers_t *channel_regs = &(dmac_reg->CHANNEL[channel]);

	/* Validate channel number */
	if (channel >= dev_data->dma_ctx.dma_channels) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	/* Check if the channel is already in use */
	if (dmac_ch_is_busy(channel_regs) == true) {
		LOG_ERR("DMA channel %d is already in use", channel);
		return -EBUSY;
	}

	/* Check if the channel is configured */
	if (dev_data->dma_channel_config[channel].is_configured != true) {
		LOG_ERR("DMA descriptors not configured for channel : %d", channel);
		return -EINVAL;
	}

	if (channel_regs->DMA_CHNXT != DMAC_NULL) {
		/* Enable loading of linked list descriptor */
		channel_regs->DMA_CHCTRLA |= DMA_CHCTRLA_LLEN_Msk;
	} else {
		/* Start is called after reload. So, proceed with current channel setting. */
		channel_regs->DMA_CHCTRLA |= DMA_CHCTRLA_ENABLE_Msk;
		if ((DMA_CHCTRLB_TRIG_Msk & channel_regs->DMA_CHCTRLB) == 0) {
			/* Trigger via software */
			channel_regs->DMA_CHCTRLA |= DMA_CHCTRLA_SWFRC_Msk;
		}
	}

	return 0;
}

/* Stop a dma transfer */
static int dma_mchp_stop(const struct device *dev, uint32_t channel)
{
	struct dma_mchp_dev_data *const dev_data = dev->data;
	dma_registers_t *dmac_reg = ((const struct dma_mchp_dev_config *)(dev)->config)->regs;
	dma_channel_registers_t *channel_regs = &(dmac_reg->CHANNEL[channel]);
	uint32_t controlb_save;

	/* Validate the DMA channel */
	if (channel >= dev_data->dma_ctx.dma_channels) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	/* Disable the channel */
	channel_regs->DMA_CHCTRLA &= ~(DMA_CHCTRLA_ENABLE_Msk | DMA_CHCTRLA_LLEN_Msk);
	if (dmac_ch_is_busy(channel_regs) == true) {
		/* Save channel registers */
		controlb_save = channel_regs->DMA_CHCTRLB;

		/* clear Cell Auto Start Enable */
		channel_regs->DMA_CHCTRLB &= ~DMA_CHCTRLB_CASTEN_Msk;
		if (WAIT_FOR(dmac_ch_is_busy(channel_regs) == false, DMAC_TIMEOUT_VALUE_US,
			     k_busy_wait(1)) == false) {
			LOG_ERR("Timeout for clearing dma busy status");
			return -ETIMEDOUT;
		}

		/* Restore channel registers */
		channel_regs->DMA_CHCTRLB = controlb_save;
	}

	return 0;
}

/* Reload dma configuration with new source and destination addresses, and size */
static int dma_mchp_reload(const struct device *dev, uint32_t channel, uint32_t src, uint32_t dst,
			   size_t size)
{
	/* Get dev data */
	struct dma_mchp_dev_data *const dev_data = dev->data;
	dma_registers_t *dmac_reg = ((const struct dma_mchp_dev_config *)(dev)->config)->regs;
	dma_channel_registers_t *channel_regs = &(dmac_reg->CHANNEL[channel]);

	/* Validate channel number */
	if (channel >= dev_data->dma_ctx.dma_channels) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	/* Check if the channel is already in use */
	if (dmac_ch_is_busy(channel_regs) == true) {
		LOG_ERR("DMA channel:%d is currently busy", channel);
		return -EBUSY;
	}

	/* Check if the channel is configured */
	if (dev_data->dma_channel_config[channel].is_configured != true) {
		LOG_ERR("DMA descriptors not configured for channel : %d", channel);
		return -EINVAL;
	}

	/* Reloads DMA with the specified source, destination, and size. */
	if ((channel_regs->DMA_CHNXT != DMAC_NULL) && (channel_regs->DMA_CHNXT != 0)) {
		dma_descriptor_registers_t *desc =
			(dma_descriptor_registers_t *)(channel_regs->DMA_CHNXT);

		desc->DMA_BDSSA = src;
		desc->DMA_BDDSA = dst;
		desc->DMA_BDXSIZ &= ~DMA_BDXSIZ_BLKSZ_Msk;
		desc->DMA_BDXSIZ |= DMA_BDXSIZ_BLKSZ(size);
	} else {
		channel_regs->DMA_CHSSA = src;
		channel_regs->DMA_CHDSA = dst;
		channel_regs->DMA_CHXSIZ &= ~(DMA_CHXSIZ_BLKSZ_Msk);
		channel_regs->DMA_CHXSIZ |= DMA_CHXSIZ_BLKSZ(size);

		/* Enable block transfer complete interrupt */
		channel_regs->DMA_CHINTENSET = DMA_CHINTENSET_BC_Msk;
	}

	return 0;
}

/* Suspend a dma transfer */
static int dma_mchp_suspend(const struct device *dev, uint32_t channel)
{
	struct dma_mchp_dev_data *const dev_data = dev->data;
	dma_registers_t *dmac_reg = ((const struct dma_mchp_dev_config *)(dev)->config)->regs;
	dma_channel_registers_t *channel_regs = &(dmac_reg->CHANNEL[channel]);

	/* Validate the DMA channel */
	if (channel >= dev_data->dma_ctx.dma_channels) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	/* Suspend the specified DMA channel */
	channel_regs->DMA_CHCTRLA &= ~(DMA_CHCTRLA_ENABLE_Msk);

	return 0;
}

/* Resume a DMA transfer that was previously suspended */
static int dma_mchp_resume(const struct device *dev, uint32_t channel)
{
	/* Get dev data */
	struct dma_mchp_dev_data *const dev_data = dev->data;
	dma_registers_t *dmac_reg = ((const struct dma_mchp_dev_config *)(dev)->config)->regs;
	dma_channel_registers_t *channel_regs = &(dmac_reg->CHANNEL[channel]);

	/* Validate the DMA channel */
	if (channel >= dev_data->dma_ctx.dma_channels) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	/* Resume the specified DMA channel */
	channel_regs->DMA_CHCTRLA |= DMA_CHCTRLA_ENABLE_Msk;

	if (channel_regs->DMA_CHNXT != DMAC_NULL) {
		/* Enable loading of linked list descriptor */
		channel_regs->DMA_CHCTRLA |= DMA_CHCTRLA_LLEN_Msk;
	}

	return 0;
}

/* Check the status of a DMA channel */
static int dma_mchp_get_status(const struct device *dev, uint32_t channel, struct dma_status *stat)
{
	/* Get dev data */
	struct dma_mchp_dev_data *const dev_data = dev->data;
	dma_registers_t *dmac_reg = ((const struct dma_mchp_dev_config *)(dev)->config)->regs;
	dma_channel_registers_t *channel_regs = &(dmac_reg->CHANNEL[channel]);

	/* Validate the DMA channel */
	if (channel >= dev_data->dma_ctx.dma_channels) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	/* Check if the channel is busy */
	stat->busy = dmac_ch_is_busy(channel_regs);

	/* Get total copied length */
	stat->total_copied = channel_regs->DMA_CHSTATBC;

	/* Calculate pending length */
	stat->pending_length =
		((channel_regs->DMA_CHXSIZ & DMA_CHXSIZ_BLKSZ_Msk) >> DMA_CHXSIZ_BLKSZ_Pos) -
		stat->total_copied;

	/* Other fields are not available. Reset their values to zero. */
	stat->free = 0;
	stat->write_position = 0;
	stat->read_position = 0;

	return 0;
}

/*
 * This function is used by the DMA framework to determine whether a given
 * channel is suitable for allocation based on the optional user-provided
 * filter parameter.
 *
 * If no filter parameter is provided (i.e., filter_param is NULL), the
 * function returns true, allowing any available channel to be selected.
 * If a filter parameter is provided, it must point to a uint32_t value
 * representing the desired channel number. The function returns true only
 * if the current channel matches the requested one.
 */
static bool dma_mchp_chan_filter(const struct device *dev, int channel, void *filter_param)
{
	uint32_t requested_channel;
	bool ret;

	/* If no specific channel is requested, allow any available channel */
	if (filter_param == NULL) {
		return true;
	}

	requested_channel = *(uint32_t *)filter_param;
	/* Allow only if current channel matches the requested one */
	ret = (bool)(channel == requested_channel);

	return ret;
}

/* Retrieve attributes related to DMA hardware */
static int dma_mchp_get_attribute(const struct device *dev, uint32_t type, uint32_t *value)
{
	/* Device is not used in current logic, but kept for API compatibility */
	ARG_UNUSED(dev);

	switch (type) {
	case DMA_MCHP_ATTR_BUFFER_ADDRESS_ALIGNMENT:
		*value = DMAC_BUF_ADDR_ALIGNMENT;
		break;

	case DMA_MCHP_ATTR_BUFFER_SIZE_ALIGNMENT:
		*value = DMAC_BUF_SIZE_ALIGNMENT;
		break;

	case DMA_MCHP_ATTR_COPY_ALIGNMENT:
		*value = DMAC_COPY_ALIGNMENT;
		break;

	case DMA_MCHP_ATTR_MAX_BLOCK_COUNT:
		*value = DMAC_MAX_BLOCK_COUNT;
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

/* Freeup the allocated descriptors. */
void dma_mchp_chan_release(const struct device *dev, uint32_t channel)
{
	struct dma_mchp_dev_data *const dev_data = dev->data;
	struct dma_mchp_channel_config *channel_config_data;

	channel_config_data = &dev_data->dma_channel_config[channel];
	dmac_desc_freeall(dev, channel_config_data);
	channel_config_data->cb = NULL;
	channel_config_data->user_data = NULL;
	channel_config_data->is_configured = false;
}

/* Handle DMA interrupts */
static void dma_isr_handler(const struct device *dev, uint32_t status)
{
	struct dma_mchp_dev_data *const dev_data = ((struct dma_mchp_dev_data *const)(dev)->data);
	dma_registers_t *dmac_reg = ((const struct dma_mchp_dev_config *)(dev)->config)->regs;
	uint32_t channel = 0, channel_interrupt_flags, cb_ret, channel_enabled_interrupts;
	dma_descriptor_registers_t *desc, *desc_head;

	for (int nibs = 0; nibs < DMAC_CH_NIBS; nibs++) {
		if ((status & (DMAC_NIBS_MASK << channel)) == 0) {
			channel += DMAC_NIBS_BIT_COUNT;
			continue;
		}
		for (int i = 0; i < DMAC_NIBS_BIT_COUNT; i++) {
			struct dma_mchp_channel_config *channel_config_data =
				&(dev_data->dma_channel_config[channel]);
			dma_channel_registers_t *channel_regs = &(dmac_reg->CHANNEL[channel]);

			if ((status & (1 << channel)) == 0) {
				channel++;
				continue;
			}

			/* Read and clear DMA channel interrupt flags */
			channel_interrupt_flags = channel_regs->DMA_CHINTF;
			channel_regs->DMA_CHINTF = channel_interrupt_flags;

			/* Always update for Read and Write error interrupts */
			channel_enabled_interrupts = channel_interrupt_flags &
						     (channel_regs->DMA_CHINTENSET |
						      (DMA_CHINTF_WRE_Msk | DMA_CHINTF_RDE_Msk));
			if ((channel_enabled_interrupts &
			     (DMA_CHINTF_WRE_Msk | DMA_CHINTF_RDE_Msk)) != 0U) {
				/* A write error or read error event has been detected */
				cb_ret = -EIO;

				/* Disable the channel */
				channel_regs->DMA_CHCTRLA &=
					~(DMA_CHCTRLA_ENABLE_Msk | DMA_CHCTRLA_LLEN_Msk);
				channel_regs->DMA_CHCTRLB &= ~DMA_CHCTRLB_CASTEN_Msk;
				dmac_desc_freeall(dev, channel_config_data);
			} else if ((channel_enabled_interrupts & DMA_CHINTF_BC_Msk) != 0U) {
				/* A block transfer has been completed */
				cb_ret = DMA_STATUS_BLOCK;
				desc_head = channel_config_data->desc_head;
				if ((!channel_config_data->is_cyclic) &&
				    (desc_head != DMAC_NULLP)) {
					desc = (dma_descriptor_registers_t *)(desc_head->DMA_BDNXT);
					dmac_desc_freeto_pool(dev, desc_head);
					channel_config_data->desc_head = desc;
				}
			} else if ((channel_enabled_interrupts & DMA_CHINTF_LL_Msk) != 0U) {
				/* All blocks are completed */
				cb_ret = DMA_STATUS_COMPLETE;
				dmac_desc_freeall(dev, channel_config_data);
			} else {
				cb_ret = -EBUSY;
			}

			if (channel_config_data->cb != NULL) {
				channel_config_data->cb(dev, channel_config_data->user_data,
							channel, cb_ret);
			}

			channel++;
		}
	}
}

/* isr_0 to isr_3, handle DMA interrupts for priority 1 to 4 respectively. */
static void dma_mchp_isr_0(const struct device *dev)
{
	dma_isr_handler(dev,
			((const struct dma_mchp_dev_config *)(dev)->config)->regs->DMA_INTSTAT1);
}

static void dma_mchp_isr_1(const struct device *dev)
{
	dma_isr_handler(dev,
			((const struct dma_mchp_dev_config *)(dev)->config)->regs->DMA_INTSTAT2);
}

static void dma_mchp_isr_2(const struct device *dev)
{
	dma_isr_handler(dev,
			((const struct dma_mchp_dev_config *)(dev)->config)->regs->DMA_INTSTAT3);
}

static void dma_mchp_isr_3(const struct device *dev)
{
	dma_isr_handler(dev,
			((const struct dma_mchp_dev_config *)(dev)->config)->regs->DMA_INTSTAT4);
}

/* Initialize DMAC */
static int dma_mchp_init(const struct device *dev)
{
	const struct dma_mchp_dev_config *dev_cfg = dev->config;
	dma_registers_t *dmac_reg = dev_cfg->regs;
	int ret;

	/* Enable DMA clock */
	ret = clock_control_on(dev_cfg->clock_dev, dev_cfg->mclk_sys);
	if (ret < 0 && ret != -EALREADY) {
		LOG_ERR("Failed to enable MCLK for DMA: %d", ret);
		return ret;
	}

	/* Enable the DMA controller */
	dmac_reg->DMA_CTRLA = DMA_CTRLA_ENABLE(1);

	/* Configure DMA interrupt */
	dev_cfg->irq_config();

	/* Initialize descriptor pool */
	dmac_desc_pool_init(dev);

	return 0;
}

/* DMA driver API structure. */
static DEVICE_API(dma, dma_mchp_api) = {
	.config = dma_mchp_config,
	.start = dma_mchp_start,
	.stop = dma_mchp_stop,
	.reload = dma_mchp_reload,
	.get_status = dma_mchp_get_status,
	.suspend = dma_mchp_suspend,
	.resume = dma_mchp_resume,
	.chan_filter = dma_mchp_chan_filter,
	.get_attribute = dma_mchp_get_attribute,
	.chan_release = dma_mchp_chan_release,
};

/* Declare the DMA IRQ connection handler for a specific instance. */
#define DMA_MCHP_IRQ_HANDLER_DECL(n) static void mchp_dma_irq_connect_##n(void)

/* Enable IRQ lines for the DMA controller. */
#define DMA_MCHP_IRQ_CONNECT(idx, n)                                                               \
	IF_ENABLED(DT_INST_IRQ_HAS_IDX(n, idx), (                                                  \
		/* Connect the IRQ to the DMA ISR */                                               \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, idx, irq),                                       \
			DT_INST_IRQ_BY_IDX(n, idx, priority),                                      \
			dma_mchp_isr_##idx,                                                        \
			DEVICE_DT_INST_GET(n), 0);                                                 \
		irq_enable(DT_INST_IRQ_BY_IDX(n, idx, irq));                                       \
	))

/* Define the DMA IRQ handler function for a given instance. */
#define DMA_MCHP_IRQ_HANDLER(n)                                                                    \
	static void mchp_dma_irq_connect_##n(void)                                                 \
	{                                                                                          \
		/** Connect all IRQs for this instance */                                          \
		LISTIFY(DT_NUM_IRQS(DT_DRV_INST(n)),          \
			DMA_MCHP_IRQ_CONNECT,                 \
			(),                                   \
			n)                                   \
	}

/* DMA runtime data structure. */
#define DMA_MCHP_DATA_DEFN(n)                                                                      \
	ATOMIC_DEFINE(dma_mchp_atomic##n, DT_INST_PROP(n, dma_channels));                          \
	static struct dma_mchp_channel_config                                                      \
		dma_channel_config_##n[DT_INST_PROP(n, dma_channels)];                             \
	static struct dma_mchp_dev_data dma_mchp_dev_data_##n = {                                  \
		.dma_ctx =                                                                         \
			{                                                                          \
				.magic = DMA_MAGIC,                                                \
				.atomic = dma_mchp_atomic##n,                                      \
				.dma_channels = DT_INST_PROP(n, dma_channels),                     \
			},                                                                         \
		.dma_channel_config = dma_channel_config_##n,                                      \
	};

/* DMA device configuration structure. */
#define DMA_MCHP_CONFIG_DEFN(n)                                                                    \
	static const struct dma_mchp_dev_config dma_mchp_dev_config_##n = {                        \
		.regs = ((dma_registers_t *)DT_INST_REG_ADDR(n)),                                  \
		.mclk_sys = (void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, mclk, subsystem)),             \
		.irq_config = mchp_dma_irq_connect_##n,                                            \
		.clock_dev = DEVICE_DT_GET(DT_NODELABEL(clock))};

/* Define and initialize the DMA device. */
#define DMA_MCHP_DEVICE_INIT(n)                                                                    \
	DMA_MCHP_IRQ_HANDLER_DECL(n);                                                              \
	DMA_MCHP_DATA_DEFN(n);                                                                     \
	DMA_MCHP_CONFIG_DEFN(n);                                                                   \
	DEVICE_DT_INST_DEFINE(n, &dma_mchp_init, NULL, &dma_mchp_dev_data_##n,                     \
			      &dma_mchp_dev_config_##n, PRE_KERNEL_1, CONFIG_DMA_INIT_PRIORITY,    \
			      &dma_mchp_api);                                                      \
	DMA_MCHP_IRQ_HANDLER(n)

DT_INST_FOREACH_STATUS_OKAY(DMA_MCHP_DEVICE_INIT);
