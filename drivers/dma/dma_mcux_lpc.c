/*
 * Copyright (c) 2020-2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Common part of DMA drivers for some NXP SoC.
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <soc.h>
#include <zephyr/drivers/dma.h>
#include <fsl_dma.h>
#include <fsl_inputmux.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/drivers/dma/dma_mcux_lpc.h>

#define DT_DRV_COMPAT nxp_lpc_dma

LOG_MODULE_REGISTER(dma_mcux_lpc, CONFIG_DMA_LOG_LEVEL);

struct dma_mcux_lpc_config {
	DMA_Type *base;
	uint32_t otrig_base_address;
	uint32_t itrig_base_address;
	uint8_t num_of_channels;
	uint8_t num_of_otrigs;
	void (*irq_config_func)(const struct device *dev);
};

struct channel_data {
	SDK_ALIGN(dma_descriptor_t dma_descriptor_table[CONFIG_DMA_MCUX_LPC_NUMBER_OF_DESCRIPTORS],
		  FSL_FEATURE_DMA_LINK_DESCRIPTOR_ALIGN_SIZE);
	dma_handle_t dma_handle;
	const struct device *dev;
	void *user_data;
	dma_callback_t dma_callback;
	enum dma_channel_direction dir;
	uint8_t src_inc;
	uint8_t dst_inc;
	dma_descriptor_t *curr_descriptor;
	uint8_t num_of_descriptors;
	bool descriptors_queued;
	uint32_t width;
	bool busy;
};

struct dma_otrig {
	int8_t source_channel;
	int8_t linked_channel;
};

struct dma_mcux_lpc_dma_data {
	struct channel_data *channel_data;
	struct dma_otrig *otrig_array;
	int8_t *channel_index;
	uint8_t num_channels_used;
};

struct k_spinlock configuring_otrigs;

#define NXP_LPC_DMA_MAX_XFER ((DMA_CHANNEL_XFERCFG_XFERCOUNT_MASK >> \
			      DMA_CHANNEL_XFERCFG_XFERCOUNT_SHIFT) + 1)

#define DEV_BASE(dev) \
	((DMA_Type *)((const struct dma_mcux_lpc_config *const)(dev)->config)->base)

#define DEV_CHANNEL_DATA(dev, ch)                                              \
	((struct channel_data *)(&(((struct dma_mcux_lpc_dma_data *)dev->data)->channel_data[ch])))

#define DEV_DMA_HANDLE(dev, ch)                                               \
		((dma_handle_t *)(&(DEV_CHANNEL_DATA(dev, ch)->dma_handle)))

#define EMPTY_OTRIG -1

static void nxp_lpc_dma_callback(dma_handle_t *handle, void *param,
			      bool transferDone, uint32_t intmode)
{
	int ret = -EIO;
	struct channel_data *data = (struct channel_data *)param;
	uint32_t channel = handle->channel;

	if (intmode == kDMA_IntError) {
		DMA_AbortTransfer(handle);
	} else if (intmode == kDMA_IntA) {
		ret = DMA_STATUS_BLOCK;
	} else {
		ret = DMA_STATUS_COMPLETE;
	}

	data->busy = DMA_ChannelIsBusy(data->dma_handle.base, channel);

	if (data->dma_callback) {
		data->dma_callback(data->dev, data->user_data, channel, ret);
	}
}

/* Handles DMA interrupts and dispatches to the individual channel */
static void dma_mcux_lpc_irq_handler(const struct device *dev)
{
	DMA_IRQHandle(DEV_BASE(dev));
/*
 * Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store
 * immediate overlapping exception return operation might vector
 * to incorrect interrupt
 */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
	barrier_dsync_fence_full();
#endif
}

#ifdef CONFIG_SOC_SERIES_RW6XX
static inline void rw6xx_dma_addr_fixup(struct dma_block_config *block)
{
	/* RW6xx AHB design does not route DMA engine through FlexSPI CACHE.
	 * Therefore, to use DMA from the FlexSPI space we must adjust the
	 * source address to use the non cached FlexSPI region.
	 * FlexSPI cached region is at 0x800_0000 (nonsecure) or 0x1800_0000
	 * (secure). We move the address into non cached region, which is at
	 * 0x4800_0000 or 0x5800_000.
	 */
	if (((block->source_address & 0xF8000000) == 0x18000000) ||
	  ((block->source_address & 0xF8000000) == 0x8000000)) {
		block->source_address = block->source_address + 0x40000000;
	}
	if (((block->dest_address & 0xF8000000) == 0x18000000) ||
	  ((block->dest_address & 0xF8000000) == 0x8000000)) {
		block->dest_address = block->dest_address + 0x40000000;
	}

}
#endif

static int dma_mcux_lpc_queue_descriptors(struct channel_data *data,
					   struct dma_block_config *block,
					   uint8_t src_inc,
					   uint8_t dest_inc,
					   bool callback_en)
{
	uint32_t xfer_config = 0U;
	dma_descriptor_t *next_descriptor = NULL;
	uint32_t width = data->width;
	uint32_t max_xfer_bytes = NXP_LPC_DMA_MAX_XFER * width;
	bool setup_extra_descriptor = false;
	/* intA is used to indicate transfer of a block */
	uint8_t enable_a_interrupt;
	/* intB is used to indicate complete transfer of the list of blocks */
	uint8_t enable_b_interrupt;
	uint8_t reload;
	struct dma_block_config local_block;
	bool last_block = false;

	memcpy(&local_block, block, sizeof(struct dma_block_config));

	do {
		/* Descriptors are queued during dma_configure, do not add more
		 * during dma_reload.
		 */
		if (!data->descriptors_queued) {
			/* Increase the number of descriptors queued */
			data->num_of_descriptors++;

			if (data->num_of_descriptors >= CONFIG_DMA_MCUX_LPC_NUMBER_OF_DESCRIPTORS) {
				return -ENOMEM;
			}
			/* Do we need to queue additional DMA descriptors for this block */
			if ((local_block.block_size > max_xfer_bytes) ||
			    (local_block.next_block != NULL)) {
				/* Allocate DMA descriptors */
				next_descriptor =
					&data->dma_descriptor_table[data->num_of_descriptors];
			} else {
				/* Check if this is the last block to transfer */
				if (local_block.next_block == NULL) {
					last_block = true;
					/* Last descriptor, check if we should setup a
					 * circular chain
					 */
					if (!local_block.source_reload_en) {
						/* No more descriptors */
						next_descriptor = NULL;
					} else if (data->num_of_descriptors == 1) {
						/* Allocate one more descriptors for
						 * ping-pong transfer
						 */
						next_descriptor = &data->dma_descriptor_table[
							data->num_of_descriptors];

						setup_extra_descriptor = true;
					} else {
						/* Loop back to the head */
						next_descriptor = data->dma_descriptor_table;
					}
				}
			}
		} else {
			/* Descriptors have already been allocated, reuse them as this
			 * is called from a reload function
			 */
			next_descriptor = data->curr_descriptor->linkToNextDesc;
		}

		/* SPI TX transfers need to queue a DMA descriptor to
		 * indicate an end of transfer. Source or destination
		 * address does not need to be change for these
		 * transactions and the transfer width is 4 bytes
		 */
		if ((local_block.source_addr_adj == DMA_ADDR_ADJ_NO_CHANGE) &&
			(local_block.dest_addr_adj == DMA_ADDR_ADJ_NO_CHANGE)) {
			src_inc = 0;
			dest_inc = 0;
			width = sizeof(uint32_t);
		}

		/* Fire an interrupt after the whole block has been transferred */
		if (local_block.block_size > max_xfer_bytes) {
			enable_a_interrupt = 0;
			enable_b_interrupt = 0;
		} else {
			/* Use intB when this is the end of the block list and transfer */
			if (last_block) {
				enable_a_interrupt = 0;
				enable_b_interrupt = 1;
			} else {
				/* Use intA when we need an interrupt per block
				 * Enable or disable intA based on user configuration
				 */
				enable_a_interrupt = callback_en;
				enable_b_interrupt = 0;
			}
		}

		/* Reload if we have more descriptors */
		if (next_descriptor) {
			reload = 1;
		} else {
			reload = 0;
		}

		/* Enable interrupt and reload for the descriptor */
		xfer_config = DMA_CHANNEL_XFER(reload, 0UL, enable_a_interrupt,
					enable_b_interrupt,
					width,
					src_inc,
					dest_inc,
					MIN(local_block.block_size, max_xfer_bytes));

#ifdef CONFIG_SOC_SERIES_RW6XX
		rw6xx_dma_addr_fixup(&local_block);
#endif
		DMA_SetupDescriptor(data->curr_descriptor,
				xfer_config,
				(void *)local_block.source_address,
				(void *)local_block.dest_address,
				(void *)next_descriptor);

		data->curr_descriptor = next_descriptor;

		if (local_block.block_size > max_xfer_bytes) {
			local_block.block_size -= max_xfer_bytes;
			if (src_inc) {
				local_block.source_address += max_xfer_bytes;
			}
			if (dest_inc) {
				local_block.dest_address += max_xfer_bytes;
			}
		} else {
			local_block.block_size = 0;
		}
	} while (local_block.block_size > 0);

	/* If an additional descriptor is queued for a certain case, set it up here.
	 */
	if (setup_extra_descriptor) {
		/* Increase the number of descriptors queued */
		data->num_of_descriptors++;

		/* Loop back to the head */
		next_descriptor = data->dma_descriptor_table;

		/* Leave curr pointer unchanged so we start queuing new data from
		 * this descriptor
		 */
		/* Enable or disable interrupt based on user request.
		 * Reload for the descriptor.
		 */
		xfer_config = DMA_CHANNEL_XFER(1UL, 0UL, callback_en, 0U,
					width,
					src_inc,
					dest_inc,
					MIN(local_block.block_size, max_xfer_bytes));
		/* Mark this as invalid */
		xfer_config &= ~DMA_CHANNEL_XFERCFG_CFGVALID_MASK;
#ifdef CONFIG_SOC_SERIES_RW6XX
		rw6xx_dma_addr_fixup(&local_block);
#endif
		DMA_SetupDescriptor(data->curr_descriptor,
				xfer_config,
				(void *)local_block.source_address,
				(void *)local_block.dest_address,
				(void *)next_descriptor);
	}

	return 0;
}

static void dma_mcux_lpc_clear_channel_data(struct channel_data *data)
{
	data->dma_callback = NULL;
	data->dir = 0;
	data->src_inc = 0;
	data->dst_inc = 0;
	data->descriptors_queued = false;
	data->num_of_descriptors = 0;
	data->curr_descriptor = NULL;
	data->width = 0;
}

/* Configure a channel */
static int dma_mcux_lpc_configure(const struct device *dev, uint32_t channel,
				  struct dma_config *config)
{
	const struct dma_mcux_lpc_config *dev_config;
	dma_handle_t *p_handle;
	uint32_t xfer_config = 0U;
	struct channel_data *data;
	struct dma_mcux_lpc_dma_data *dma_data;
	struct dma_block_config *block_config;
	uint32_t virtual_channel;
	uint8_t otrig_index;
	uint8_t src_inc = 1, dst_inc = 1;
	bool is_periph = true;
	uint8_t width;
	uint32_t max_xfer_bytes;
	uint8_t reload = 0;
	bool complete_callback;

	if (NULL == dev || NULL == config) {
		return -EINVAL;
	}

	dev_config = dev->config;
	dma_data = dev->data;
	block_config = config->head_block;
	/* The DMA controller deals with just one transfer
	 * size, though the API provides separate sizes
	 * for source and dest. So assert that the source
	 * and dest sizes are the same.
	 */
	assert(config->dest_data_size == config->source_data_size);
	width = config->dest_data_size;

	/* If skip is set on both source and destination
	 * then skip by the same amount on both sides
	 */
	if (block_config->source_gather_en && block_config->dest_scatter_en) {
		assert(block_config->source_gather_interval ==
		       block_config->dest_scatter_interval);
	}

	max_xfer_bytes = NXP_LPC_DMA_MAX_XFER * width;

	/*
	 * Check if circular mode is requested.
	 */
	if (config->head_block->source_reload_en ||
	    config->head_block->dest_reload_en) {
		reload = 1;
	}

	/* Check if have a free slot to store DMA channel data */
	if (dma_data->num_channels_used > dev_config->num_of_channels) {
		LOG_ERR("out of DMA channel %d", channel);
		return -EINVAL;
	}

	/* Check if the dma channel number is valid */
	if (channel >= dev_config->num_of_channels) {
		LOG_ERR("invalid DMA channel number %d", channel);
		return -EINVAL;
	}

	if (config->source_data_size != 4U &&
		config->source_data_size != 2U &&
		config->source_data_size != 1U) {
		LOG_ERR("Source unit size error, %d", config->source_data_size);
		return -EINVAL;
	}

	if (config->dest_data_size != 4U &&
		config->dest_data_size != 2U &&
		config->dest_data_size != 1U) {
		LOG_ERR("Dest unit size error, %d", config->dest_data_size);
		return -EINVAL;
	}

	switch (config->channel_direction) {
	case MEMORY_TO_MEMORY:
		is_periph = false;
		if (block_config->source_gather_en) {
			src_inc = block_config->source_gather_interval / width;
			/* The current controller only supports incrementing the
			 * source and destination up to 4 time transfer width
			 */
			if ((src_inc > 4) || (src_inc == 3)) {
				return -EINVAL;
			}
		}

		if (block_config->dest_scatter_en) {
			dst_inc = block_config->dest_scatter_interval / width;
			/* The current controller only supports incrementing the
			 * source and destination up to 4 time transfer width
			 */
			if ((dst_inc > 4) || (dst_inc == 3)) {
				return -EINVAL;
			}
		}
		break;
	case MEMORY_TO_PERIPHERAL:
		/* Set the source increment value */
		if (block_config->source_gather_en) {
			src_inc = block_config->source_gather_interval / width;
			/* The current controller only supports incrementing the
			 * source and destination up to 4 time transfer width
			 */
			if ((src_inc > 4) || (src_inc == 3)) {
				return -EINVAL;
			}
		}

		dst_inc = 0;
		break;
	case PERIPHERAL_TO_MEMORY:
		src_inc = 0;

		/* Set the destination increment value */
		if (block_config->dest_scatter_en) {
			dst_inc = block_config->dest_scatter_interval / width;
			/* The current controller only supports incrementing the
			 * source and destination up to 4 time transfer width
			 */
			if ((dst_inc > 4) || (dst_inc == 3)) {
				return -EINVAL;
			}
		}
		break;
	default:
		LOG_ERR("not support transfer direction");
		return -EINVAL;
	}

	/* Check if user does not want to increment address */
	if (block_config->source_addr_adj == DMA_ADDR_ADJ_NO_CHANGE) {
		src_inc = 0;
	}

	if (block_config->dest_addr_adj == DMA_ADDR_ADJ_NO_CHANGE) {
		dst_inc = 0;
	}

	/* If needed, allocate a slot to store dma channel data */
	if (dma_data->channel_index[channel] == -1) {
		dma_data->channel_index[channel] = dma_data->num_channels_used;
		dma_data->num_channels_used++;
		/* Get the slot number that has the dma channel data */
		virtual_channel = dma_data->channel_index[channel];
		/* dma channel data */
		p_handle = DEV_DMA_HANDLE(dev, virtual_channel);
		data = DEV_CHANNEL_DATA(dev, virtual_channel);

		DMA_CreateHandle(p_handle, DEV_BASE(dev), channel);
		DMA_SetCallback(p_handle, nxp_lpc_dma_callback, (void *)data);
	} else {
		/* Get the slot number that has the dma channel data */
		virtual_channel = dma_data->channel_index[channel];
		/* dma channel data */
		p_handle = DEV_DMA_HANDLE(dev, virtual_channel);
		data = DEV_CHANNEL_DATA(dev, virtual_channel);
	}

	dma_mcux_lpc_clear_channel_data(data);

	data->dir = config->channel_direction;
	/* Save the increment values for the reload function */
	data->src_inc = src_inc;
	data->dst_inc = dst_inc;

	if (data->busy) {
		DMA_AbortTransfer(p_handle);
	}

	LOG_DBG("channel is %d", p_handle->channel);

	k_spinlock_key_t otrigs_key = k_spin_lock(&configuring_otrigs);

	data->width = width;

	if (config->source_chaining_en || config->dest_chaining_en) {
		/* Chaining is enabled */
		if (!dev_config->otrig_base_address || !dev_config->itrig_base_address) {
			LOG_ERR("Calling function tried to setup up channel"
			" chaining but the current platform is missing"
			" the correct trigger base addresses.");
			k_spin_unlock(&configuring_otrigs, otrigs_key);
			return -ENXIO;
		}

		LOG_DBG("link dma 0 channel %d with channel %d",
			channel, config->linked_channel);
		uint8_t is_otrig_available = 0;

		for (otrig_index = 0; otrig_index < dev_config->num_of_otrigs;
			++otrig_index) {
			if (dma_data->otrig_array[otrig_index].linked_channel == EMPTY_OTRIG ||
			    dma_data->otrig_array[otrig_index].source_channel == channel) {
				if (dma_data->otrig_array[otrig_index].source_channel == channel) {
					int ChannelToDisable =
						dma_data->otrig_array[otrig_index].linked_channel;
					DMA_DisableChannel(DEV_BASE(dev), ChannelToDisable);
					DEV_BASE(dev)->CHANNEL[ChannelToDisable].CFG &=
						~DMA_CHANNEL_CFG_HWTRIGEN_MASK;
				}
				is_otrig_available = 1;
				break;
			}
		}
		if (!is_otrig_available) {
			LOG_ERR("Calling function tried to setup up multiple"
			" channels to be configured but the dma driver has"
			" run out of OTrig Muxes");
			k_spin_unlock(&configuring_otrigs, otrigs_key);
			return -EINVAL;
		}

		/* Since INPUTMUX handles the dma signals and
		 * must be hardware triggered via the INPUTMUX
		 * hardware.
		 */
		DEV_BASE(dev)->CHANNEL[config->linked_channel].CFG |=
			DMA_CHANNEL_CFG_HWTRIGEN_MASK;

		DMA_EnableChannel(DEV_BASE(dev), config->linked_channel);

		/* Link OTrig Muxes with passed-in channels */
		INPUTMUX_AttachSignal(INPUTMUX, otrig_index,
			dev_config->otrig_base_address + channel);
		INPUTMUX_AttachSignal(INPUTMUX, config->linked_channel,
				dev_config->itrig_base_address + otrig_index);

		/* Otrig is now connected with linked channel */
		dma_data->otrig_array[otrig_index].source_channel = channel;
		dma_data->otrig_array[otrig_index].linked_channel = config->linked_channel;
	} else {
		/* Chaining is _NOT_ enabled, Freeing connected OTrig */
		for (otrig_index = 0; otrig_index < dev_config->num_of_otrigs; otrig_index++) {
			if (dma_data->otrig_array[otrig_index].linked_channel != EMPTY_OTRIG &&
			   (channel == dma_data->otrig_array[otrig_index].source_channel)) {
				int ChannelToDisable =
					dma_data->otrig_array[otrig_index].linked_channel;
				DMA_DisableChannel(DEV_BASE(dev), ChannelToDisable);
				DEV_BASE(dev)->CHANNEL[ChannelToDisable].CFG &=
					~DMA_CHANNEL_CFG_HWTRIGEN_MASK;
				dma_data->otrig_array[otrig_index].linked_channel = EMPTY_OTRIG;
				dma_data->otrig_array[otrig_index].source_channel = EMPTY_OTRIG;
				break;
			}
		}
	}

	k_spin_unlock(&configuring_otrigs, otrigs_key);

	complete_callback = config->complete_callback_en;

	/* Check if we need to queue DMA descriptors */
	if ((block_config->block_size > max_xfer_bytes) ||
		(block_config->next_block != NULL)) {
		/* Allocate a DMA descriptor */
		data->curr_descriptor = data->dma_descriptor_table;

		if (block_config->block_size > max_xfer_bytes) {
			/* Disable interrupt as this is not the entire data.
			 * Reload for the descriptor
			 */
			xfer_config = DMA_CHANNEL_XFER(1UL, 0UL, 0UL, 0UL,
					width,
					src_inc,
					dst_inc,
					max_xfer_bytes);
		} else {
			/* Enable INTA interrupt if user requested DMA for each block.
			 * Reload for the descriptor.
			 */
			xfer_config = DMA_CHANNEL_XFER(1UL, 0UL, complete_callback, 0UL,
					width,
					src_inc,
					dst_inc,
					block_config->block_size);
		}
	} else {
		/* Enable interrupt for the descriptor */
		xfer_config = DMA_CHANNEL_XFER(0UL, 0UL, 1UL, 0UL,
				width,
				src_inc,
				dst_inc,
				block_config->block_size);
	}
	/* DMA controller requires that the address be aligned to transfer size */
	assert(block_config->source_address == ROUND_UP(block_config->source_address, width));
	assert(block_config->dest_address == ROUND_UP(block_config->dest_address, width));

#ifdef CONFIG_SOC_SERIES_RW6XX
	rw6xx_dma_addr_fixup(block_config);
#endif

	DMA_SubmitChannelTransferParameter(p_handle,
					xfer_config,
					(void *)block_config->source_address,
					(void *)block_config->dest_address,
					(void *)data->curr_descriptor);

	/* Start queuing DMA descriptors */
	if (data->curr_descriptor) {
		if (block_config->block_size > max_xfer_bytes) {
			/* Queue additional DMA descriptors because the amount of data to
			 * be transferred is greater that the DMA descriptors max XFERCOUNT.
			 */
			struct dma_block_config local_block = { 0 };

			if (src_inc) {
				local_block.source_address = block_config->source_address
							     + max_xfer_bytes;
			} else {
				local_block.source_address = block_config->source_address;
			}
			if (dst_inc) {
				local_block.dest_address = block_config->dest_address
							     + max_xfer_bytes;
			} else {
				local_block.dest_address = block_config->dest_address;
			}
			local_block.block_size = block_config->block_size - max_xfer_bytes;
			local_block.next_block = block_config->next_block;
			local_block.source_reload_en = reload;

			if (block_config->next_block == NULL) {
				/* This is the last block, enable callback. */
				complete_callback = true;
			}

			if (dma_mcux_lpc_queue_descriptors(data, &local_block,
					src_inc, dst_inc, complete_callback)) {
				return -ENOMEM;
			}
		}
		/* Get the next block to transfer */
		block_config = block_config->next_block;

		while (block_config != NULL) {
			block_config->source_reload_en = reload;

			/* DMA controller requires that the address be aligned to transfer size */
			assert(block_config->source_address ==
			       ROUND_UP(block_config->source_address, width));
			assert(block_config->dest_address ==
			       ROUND_UP(block_config->dest_address, width));

			if (block_config->next_block == NULL) {
				/* This is the last block. Enable callback if not enabled. */
				complete_callback = true;
			}
			if (dma_mcux_lpc_queue_descriptors(data, block_config,
				src_inc, dst_inc, complete_callback)) {
				return -ENOMEM;
			}

			/* Get the next block and start queuing descriptors */
			block_config = block_config->next_block;
		}
		/* We have finished queuing DMA descriptors */
		data->descriptors_queued = true;
	}

	if (config->dma_slot) {
		uint32_t cfg_reg = 0;

		/* User supplied manual trigger configuration */
		if (config->dma_slot & LPC_DMA_PERIPH_REQ_EN) {
			cfg_reg |= DMA_CHANNEL_CFG_PERIPHREQEN_MASK;
		}
		if (config->dma_slot & LPC_DMA_HWTRIG_EN) {
			/* Setup hardware trigger */
			cfg_reg |= DMA_CHANNEL_CFG_HWTRIGEN_MASK;
			if (config->dma_slot & LPC_DMA_TRIGTYPE_LEVEL) {
				cfg_reg |= DMA_CHANNEL_CFG_TRIGTYPE_MASK;
			}
			if (config->dma_slot & LPC_DMA_TRIGPOL_HIGH_RISING) {
				cfg_reg |= DMA_CHANNEL_CFG_TRIGPOL_MASK;
			}
			if (config->dma_slot & LPC_DMA_TRIGBURST) {
				cfg_reg |= DMA_CHANNEL_CFG_TRIGBURST_MASK;
				cfg_reg |= DMA_CHANNEL_CFG_BURSTPOWER(
					LPC_DMA_GET_BURSTPOWER(config->dma_slot));
			}
		}
		p_handle->base->CHANNEL[p_handle->channel].CFG = cfg_reg;
	} else if (is_periph) {
		DMA_EnableChannelPeriphRq(p_handle->base, p_handle->channel);
	} else {
		DMA_DisableChannelPeriphRq(p_handle->base, p_handle->channel);
	}
	DMA_SetChannelPriority(p_handle->base, p_handle->channel, config->channel_priority);

	data->busy = false;
	if (config->dma_callback) {
		LOG_DBG("INSTALL call back on channel %d", channel);
		data->user_data = config->user_data;
		data->dma_callback = config->dma_callback;
		data->dev = dev;
	}

	return 0;
}

static int dma_mcux_lpc_start(const struct device *dev, uint32_t channel)
{
	struct dma_mcux_lpc_dma_data *dev_data = dev->data;
	int8_t virtual_channel = dev_data->channel_index[channel];
	struct channel_data *data = DEV_CHANNEL_DATA(dev, virtual_channel);

	LOG_DBG("START TRANSFER");
	LOG_DBG("DMA CTRL 0x%x", DEV_BASE(dev)->CTRL);
	data->busy = true;
	DMA_StartTransfer(DEV_DMA_HANDLE(dev, virtual_channel));
	return 0;
}

static int dma_mcux_lpc_stop(const struct device *dev, uint32_t channel)
{
	struct dma_mcux_lpc_dma_data *dev_data = dev->data;
	int8_t virtual_channel = dev_data->channel_index[channel];
	struct channel_data *data = DEV_CHANNEL_DATA(dev, virtual_channel);

	if (!data->busy) {
		return 0;
	}
	DMA_AbortTransfer(DEV_DMA_HANDLE(dev, virtual_channel));
	DMA_DisableChannel(DEV_BASE(dev), channel);

	data->busy = false;
	return 0;
}

static int dma_mcux_lpc_reload(const struct device *dev, uint32_t channel,
			       uint32_t src, uint32_t dst, size_t size)
{
	struct dma_mcux_lpc_dma_data *dev_data = dev->data;
	int8_t virtual_channel = dev_data->channel_index[channel];
	struct channel_data *data = DEV_CHANNEL_DATA(dev, virtual_channel);
	uint32_t xfer_config = 0U;

	/* DMA controller requires that the address be aligned to transfer size */
	assert(src == ROUND_UP(src, data->width));
	assert(dst == ROUND_UP(dst, data->width));

	if (!data->descriptors_queued) {
		dma_handle_t *p_handle;

		p_handle = DEV_DMA_HANDLE(dev, virtual_channel);

		/* Only one buffer, enable interrupt */
		xfer_config = DMA_CHANNEL_XFER(0UL, 0UL, 1UL, 0UL,
					data->width,
					data->src_inc,
					data->dst_inc,
					size);
		DMA_SubmitChannelTransferParameter(p_handle,
						xfer_config,
						(void *)src,
						(void *)dst,
						NULL);
	} else {
		struct dma_block_config local_block = { 0 };

		local_block.source_address = src;
		local_block.dest_address = dst;
		local_block.block_size = size;
		local_block.source_reload_en = 1;
		dma_mcux_lpc_queue_descriptors(data, &local_block,
					       data->src_inc, data->dst_inc, true);
	}

	return 0;
}

static int dma_mcux_lpc_get_status(const struct device *dev, uint32_t channel,
				   struct dma_status *status)
{
	const struct dma_mcux_lpc_config *config = dev->config;
	struct dma_mcux_lpc_dma_data *dev_data = dev->data;
	int8_t virtual_channel = dev_data->channel_index[channel];
	struct channel_data *data = DEV_CHANNEL_DATA(dev, virtual_channel);

	if (channel > config->num_of_channels) {
		return -EINVAL;
	}

	/* If channel is actually busy or the virtual channel is just not set up */
	if (data->busy && (virtual_channel != -1)) {
		status->busy = true;
		status->pending_length = DMA_GetRemainingBytes(DEV_BASE(dev), channel);
	} else {
		status->busy = false;
		status->pending_length = 0;
	}
	status->dir = data->dir;
	LOG_DBG("DMA CR 0x%x", DEV_BASE(dev)->CTRL);
	LOG_DBG("DMA INT 0x%x", DEV_BASE(dev)->INTSTAT);

	return 0;
}

static int dma_mcux_lpc_init(const struct device *dev)
{
	const struct dma_mcux_lpc_config *config = dev->config;
	struct dma_mcux_lpc_dma_data *data = dev->data;

	/* Indicate that the Otrig Muxes are not connected */
	for (int i = 0; i < config->num_of_otrigs; i++) {
		data->otrig_array[i].source_channel = EMPTY_OTRIG;
		data->otrig_array[i].linked_channel = EMPTY_OTRIG;
	}

	/*
	 * Initialize to -1 to indicate dma channel does not have a slot
	 * assigned to store dma channel data
	 */
	for (int i = 0; i < config->num_of_channels; i++) {
		data->channel_index[i] = -1;
	}

	data->num_channels_used = 0;

	DMA_Init(DEV_BASE(dev));
	INPUTMUX_Init(INPUTMUX);

	return 0;
}

static const struct dma_driver_api dma_mcux_lpc_api = {
	.config = dma_mcux_lpc_configure,
	.start = dma_mcux_lpc_start,
	.stop = dma_mcux_lpc_stop,
	.reload = dma_mcux_lpc_reload,
	.get_status = dma_mcux_lpc_get_status,
};

#define DMA_MCUX_LPC_CONFIG_FUNC(n)					\
	static void dma_mcux_lpc_config_func_##n(const struct device *dev)	\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
			    DT_INST_IRQ(n, priority),			\
			    dma_mcux_lpc_irq_handler, DEVICE_DT_INST_GET(n), 0);\
									\
		irq_enable(DT_INST_IRQN(n));				\
	}
#define DMA_MCUX_LPC_IRQ_CFG_FUNC_INIT(n)				\
	.irq_config_func = dma_mcux_lpc_config_func_##n
#define DMA_MCUX_LPC_INIT_CFG(n)					\
	DMA_MCUX_LPC_DECLARE_CFG(n,					\
				 DMA_MCUX_LPC_IRQ_CFG_FUNC_INIT(n))

#define DMA_MCUX_LPC_NUM_USED_CHANNELS(n)				\
	COND_CODE_0(CONFIG_DMA_MCUX_LPC_NUMBER_OF_CHANNELS_ALLOCATED,	\
		    (DT_INST_PROP(n, dma_channels)),			\
		    (MIN(CONFIG_DMA_MCUX_LPC_NUMBER_OF_CHANNELS_ALLOCATED,	\
			DT_INST_PROP(n, dma_channels))))

#define DMA_MCUX_LPC_DECLARE_CFG(n, IRQ_FUNC_INIT)			\
static const struct dma_mcux_lpc_config dma_##n##_config = {		\
	.base = (DMA_Type *)DT_INST_REG_ADDR(n),			\
	.num_of_channels = DT_INST_PROP(n, dma_channels),		\
	.num_of_otrigs = DT_INST_PROP_OR(n, nxp_dma_num_of_otrigs, 0),			\
	.otrig_base_address = DT_INST_PROP_OR(n, nxp_dma_otrig_base_address, 0x0),	\
	.itrig_base_address = DT_INST_PROP_OR(n, nxp_dma_itrig_base_address, 0x0),	\
	IRQ_FUNC_INIT							\
}

#define DMA_INIT(n) \
									\
	static const struct dma_mcux_lpc_config dma_##n##_config;	\
									\
	static struct channel_data dma_##n##_channel_data_arr		\
			[DMA_MCUX_LPC_NUM_USED_CHANNELS(n)] = {0};	\
									\
	static struct dma_otrig dma_##n##_otrig_arr			\
			[DT_INST_PROP_OR(n, nxp_dma_num_of_otrigs, 0)]; \
									\
	static int8_t							\
		dma_##n##_channel_index_arr				\
				[DT_INST_PROP(n, dma_channels)] = {0};	\
									\
	static struct dma_mcux_lpc_dma_data dma_data_##n = {		\
		.channel_data = dma_##n##_channel_data_arr,		\
		.channel_index = dma_##n##_channel_index_arr,		\
		.otrig_array = dma_##n##_otrig_arr,			\
	};								\
									\
	DEVICE_DT_INST_DEFINE(n,					\
			    &dma_mcux_lpc_init,				\
			    NULL,					\
			    &dma_data_##n, &dma_##n##_config,		\
			    PRE_KERNEL_1, CONFIG_DMA_INIT_PRIORITY,	\
			    &dma_mcux_lpc_api);				\
									\
	DMA_MCUX_LPC_CONFIG_FUNC(n)					\
	DMA_MCUX_LPC_INIT_CFG(n);

DT_INST_FOREACH_STATUS_OKAY(DMA_INIT)
