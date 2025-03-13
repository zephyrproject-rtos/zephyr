/*
 * Copyright (c) 2023 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief DMA driver for Infineon CAT1 MCU family.
 */

#define DT_DRV_COMPAT infineon_cat1_dma

#include <zephyr/device.h>
#include <soc.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

#include <cy_pdl.h>
#include <cyhal_dma_dw.h>

#if CYHAL_DRIVER_AVAILABLE_SYSPM && CONFIG_PM
#include "cyhal_syspm_impl.h"
#endif

#include <zephyr/devicetree.h>
LOG_MODULE_REGISTER(ifx_cat1_dma, CONFIG_DMA_LOG_LEVEL);

#define CH_NUM               32
#define DESCRIPTOR_POOL_SIZE CH_NUM + 5 /* TODO: add parameter to Kconfig */
#define DMA_LOOP_X_COUNT_MAX CY_DMA_LOOP_COUNT_MAX
#define DMA_LOOP_Y_COUNT_MAX CY_DMA_LOOP_COUNT_MAX

#if CONFIG_SOC_FAMILY_INFINEON_CAT1B
/* For CAT1B we must use SBUS instead CBUS when operate with
 * flash area, so convert address from CBUS to SBUS
 */
#define IFX_CAT1B_FLASH_SBUS_ADDR (0x60000000)
#define IFX_CAT1B_FLASH_CBUS_ADDR (0x8000000)
#define IFX_CAT1_DMA_SRC_ADDR(v)                                                                   \
	(void *)(((uint32_t)v & IFX_CAT1B_FLASH_CBUS_ADDR)                                         \
			 ? (IFX_CAT1B_FLASH_SBUS_ADDR + ((uint32_t)v - IFX_CAT1B_FLASH_CBUS_ADDR)) \
			 : (uint32_t)v)
#else
#define IFX_CAT1_DMA_SRC_ADDR(v) ((void *)v)
#endif

struct ifx_cat1_dma_channel {
	uint32_t channel_direction: 3;
	uint32_t error_callback_dis: 1;

	cy_stc_dma_descriptor_t *descr;
	IRQn_Type irq;

	/* store config data from dma_config structure */
	dma_callback_t callback;
	void *user_data;
};

struct ifx_cat1_dma_data {
	struct dma_context ctx;
	struct ifx_cat1_dma_channel *channels;

#if CYHAL_DRIVER_AVAILABLE_SYSPM && CONFIG_PM
	cyhal_syspm_callback_data_t syspm_callback_args;
#endif
};

struct ifx_cat1_dma_config {
	uint8_t num_channels;
	DW_Type *regs;
	void (*irq_configure)(void);
};

/* Descriptors pool */
K_MEM_SLAB_DEFINE_STATIC(ifx_cat1_dma_descriptors_pool_slab, sizeof(cy_stc_dma_descriptor_t),
			 DESCRIPTOR_POOL_SIZE, 4);

static int32_t _get_hw_block_num(DW_Type *reg_addr)
{
#if (CPUSS_DW0_PRESENT == 1)
	if ((uint32_t)reg_addr == DW0_BASE) {
		return 0;
	}
#endif

#if (CPUSS_DW1_PRESENT == 1)
	if ((uint32_t)reg_addr == DW1_BASE) {
		return 1;
	}
#endif
	return 0;
}

static int _dma_alloc_descriptor(void **descr)
{
	int ret = k_mem_slab_alloc(&ifx_cat1_dma_descriptors_pool_slab, (void **)descr, K_NO_WAIT);

	if (!ret) {
		memset(*descr, 0, sizeof(cy_stc_dma_descriptor_t));
	}

	return ret;
}

void _dma_free_descriptor(cy_stc_dma_descriptor_t *descr)
{
	k_mem_slab_free(&ifx_cat1_dma_descriptors_pool_slab, descr);
}

void _dma_free_linked_descriptors(cy_stc_dma_descriptor_t *descr)
{
	if (descr == NULL) {
		return;
	}
	cy_stc_dma_descriptor_t *descr_to_remove = descr;
	cy_stc_dma_descriptor_t *descr_to_remove_next = NULL;

	do {
		descr_to_remove_next = (cy_stc_dma_descriptor_t *)descr_to_remove->nextPtr;
		_dma_free_descriptor(descr_to_remove);
		descr_to_remove = descr_to_remove_next;

	} while (descr_to_remove);
}

int ifx_cat1_dma_ex_connect_digital(const struct device *dev, uint32_t channel,
				    cyhal_source_t source, cyhal_dma_input_t input)
{
	const struct ifx_cat1_dma_config *const cfg = dev->config;

	cyhal_dma_t dma_obj = {
		.resource.type = CYHAL_RSC_DW,
		.resource.block_num = _get_hw_block_num(cfg->regs),
		.resource.channel_num = channel,
	};

	cy_rslt_t rslt = cyhal_dma_connect_digital(&dma_obj, source, input);

	return rslt ? -EIO : 0;
}

int ifx_cat1_dma_ex_enable_output(const struct device *dev, uint32_t channel,
				  cyhal_dma_output_t output, cyhal_source_t *source)
{
	const struct ifx_cat1_dma_config *const cfg = dev->config;

	cyhal_dma_t dma_obj = {
		.resource.type = CYHAL_RSC_DW,
		.resource.block_num = _get_hw_block_num(cfg->regs),
		.resource.channel_num = channel,
	};

	cy_rslt_t rslt = cyhal_dma_enable_output(&dma_obj, output, source);

	return rslt ? -EIO : 0;
}

static cy_en_dma_data_size_t _convert_dma_data_size_z_to_pdl(struct dma_config *config)
{
	cy_en_dma_data_size_t pdl_dma_data_size = CY_DMA_BYTE;

	switch (config->source_data_size) {
	case 1:
		/* One byte */
		pdl_dma_data_size = CY_DMA_BYTE;
		break;
	case 2:
		/* Half word (two bytes) */
		pdl_dma_data_size = CY_DMA_HALFWORD;
		break;
	case 4:
		/* Full word (four bytes) */
		pdl_dma_data_size = CY_DMA_WORD;
		break;
	}
	return pdl_dma_data_size;
}

static int _convert_dma_xy_increment_z_to_pdl(uint32_t addr_adj)
{
	switch (addr_adj) {
	case DMA_ADDR_ADJ_INCREMENT:
		return 1;

	case DMA_ADDR_ADJ_DECREMENT:
		return -1;

	case DMA_ADDR_ADJ_NO_CHANGE:
		return 0;

	default:
		return 0;
	}
}

static int _initialize_descriptor(cy_stc_dma_descriptor_t *descriptor, struct dma_config *config,
				  struct dma_block_config *block_config, uint32_t block_num,
				  uint32_t bytes, uint32_t offset)
{
	cy_en_dma_status_t dma_status;
	cy_stc_dma_descriptor_config_t descriptor_config = {0u};

	/* Retrigger descriptor immediately */
	descriptor_config.retrigger = CY_DMA_RETRIG_IM;

	/* Setup Interrupt Type */
	descriptor_config.interruptType = CY_DMA_DESCR_CHAIN;

	if (((offset + bytes) == block_config->block_size) &&
	    (block_num + 1 == config->block_count)) {
		descriptor_config.channelState = CY_DMA_CHANNEL_DISABLED;
	} else {
		descriptor_config.channelState = CY_DMA_CHANNEL_ENABLED;
	}

	/* TODO: should be able to configure triggerInType/triggerOutType */
	descriptor_config.triggerOutType = CY_DMA_1ELEMENT;

	if (config->channel_direction == MEMORY_TO_MEMORY) {
		descriptor_config.triggerInType = CY_DMA_DESCR_CHAIN;
	} else {
		descriptor_config.triggerInType = CY_DMA_1ELEMENT;
	}

	/* Set data size byte / 2 bytes / word */
	descriptor_config.dataSize = _convert_dma_data_size_z_to_pdl(config);

	/* By default, transfer what the user set for dataSize. However, if transferring between
	 * memory and a peripheral, make sure the peripheral access is using words.
	 */
	descriptor_config.srcTransferSize = CY_DMA_TRANSFER_SIZE_DATA;
	descriptor_config.dstTransferSize = CY_DMA_TRANSFER_SIZE_DATA;

	if (config->channel_direction == PERIPHERAL_TO_MEMORY) {
		descriptor_config.srcTransferSize = CY_DMA_TRANSFER_SIZE_WORD;
	} else if (config->channel_direction == MEMORY_TO_PERIPHERAL) {
		descriptor_config.dstTransferSize = CY_DMA_TRANSFER_SIZE_WORD;
	}

	/* Setup destination increment for X source loop */
	descriptor_config.srcXincrement =
		_convert_dma_xy_increment_z_to_pdl(block_config->source_addr_adj);

	/* Setup destination increment for X destination loop */
	descriptor_config.dstXincrement =
		_convert_dma_xy_increment_z_to_pdl(block_config->dest_addr_adj);

	/* Setup 1D/2D descriptor for each data block */
	if (bytes >= DMA_LOOP_X_COUNT_MAX) {
		descriptor_config.descriptorType = CY_DMA_2D_TRANSFER;
		descriptor_config.xCount = DMA_LOOP_X_COUNT_MAX;
		descriptor_config.yCount = DIV_ROUND_UP(bytes, DMA_LOOP_X_COUNT_MAX);
		descriptor_config.srcYincrement =
			descriptor_config.srcXincrement * DMA_LOOP_X_COUNT_MAX;
		descriptor_config.dstYincrement =
			descriptor_config.dstXincrement * DMA_LOOP_X_COUNT_MAX;
	} else {
		descriptor_config.descriptorType = CY_DMA_1D_TRANSFER;
		descriptor_config.xCount = bytes;
		descriptor_config.yCount = 1;
		descriptor_config.srcYincrement = 0;
		descriptor_config.dstYincrement = 0;
	}

	/* Set source and destination for descriptor */
	descriptor_config.srcAddress = IFX_CAT1_DMA_SRC_ADDR(
		(block_config->source_address + (descriptor_config.srcXincrement ? offset : 0)));
	descriptor_config.dstAddress = (void *)(block_config->dest_address +
						(descriptor_config.dstXincrement ? offset : 0));

	/* initialize descriptor */
	dma_status = Cy_DMA_Descriptor_Init(descriptor, &descriptor_config);
	if (dma_status != CY_DMA_SUCCESS) {
		return -EIO;
	}

	return 0;
}

/* Configure a channel v2 */
static int ifx_cat1_dma_configure(const struct device *dev, uint32_t channel,
				  struct dma_config *config)
{
	bool use_dt_config = false;
	cy_en_dma_status_t dma_status;
	struct ifx_cat1_dma_data *data = dev->data;
	const struct ifx_cat1_dma_config *const cfg = dev->config;

	cy_stc_dma_channel_config_t channel_config = {0u};
	cy_stc_dma_descriptor_t *descriptor = NULL;
	cy_stc_dma_descriptor_t *prev_descriptor = NULL;

	if (channel >= cfg->num_channels) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	/* Support only the same data width for source and dest */
	if (config->dest_data_size != config->source_data_size) {
		LOG_ERR("Source and dest data size differ.");
		return -EINVAL;
	}

	if ((config->dest_data_size != 1) && (config->dest_data_size != 2) &&
	    (config->dest_data_size != 4)) {
		LOG_ERR("dest_data_size must be 1, 2, or 4 (%" PRIu32 ")", config->dest_data_size);
		return -EINVAL;
	}

	if (config->complete_callback_en > 1) {
		LOG_ERR("Callback on each block not implemented");
		return -ENOTSUP;
	}

	data->channels[channel].callback = config->dma_callback;
	data->channels[channel].user_data = config->user_data;
	data->channels[channel].channel_direction = config->channel_direction;
	data->channels[channel].error_callback_dis = config->error_callback_dis;

	/* Remove all allocated linked descriptors */
	_dma_free_linked_descriptors(data->channels[channel].descr);
	data->channels[channel].descr = NULL;

	/* Lock and page in the channel configuration */
	uint32_t key = irq_lock();

	struct dma_block_config *block_config = config->head_block;

	for (uint32_t i = 0u; i < config->block_count; i++) {
		uint32_t block_pending_bytes = block_config->block_size;
		uint32_t offset = 0;

		do {
			/* Configure descriptors for one block */
			uint32_t bytes;

			/* allocate new descriptor */
			if (_dma_alloc_descriptor((void **)&descriptor)) {
				LOG_ERR("Can't allocate new descriptor");
				return -EINVAL;
			}

			if (data->channels[channel].descr == NULL) {
				/* Store first descriptor in data structure */
				data->channels[channel].descr = descriptor;
			}

			/* Mendotary chain descriptors in scope of one pack */
			if (prev_descriptor != NULL) {
				Cy_DMA_Descriptor_SetNextDescriptor(prev_descriptor, descriptor);
			}

			/* Calculate bytes, block_pending_bytes for 1D/2D descriptor */
			if (block_pending_bytes <= DMA_LOOP_X_COUNT_MAX) {
				/* Calculate bytes for 1D descriptor */
				bytes = block_pending_bytes;
				block_pending_bytes = 0;
			} else {
				/* Calculate bytes for 2D descriptor */
				if (block_pending_bytes >
				    (DMA_LOOP_X_COUNT_MAX * DMA_LOOP_Y_COUNT_MAX)) {
					bytes = DMA_LOOP_X_COUNT_MAX * DMA_LOOP_Y_COUNT_MAX;
				} else {
					bytes = DMA_LOOP_Y_COUNT_MAX *
						(block_pending_bytes / DMA_LOOP_Y_COUNT_MAX);
				}
				block_pending_bytes -= bytes;
			}

			_initialize_descriptor(descriptor, config, block_config,
					       /* block_num */ i, bytes, offset);
			offset += bytes;
			prev_descriptor = descriptor;

		} while (block_pending_bytes > 0);

		block_config = block_config->next_block;
	}

	/* Set a descriptor for the specified DMA channel */
	channel_config.descriptor = data->channels[channel].descr;

	/* Set a priority for the DMA channel */
	if (use_dt_config == false) {
		Cy_DMA_Channel_SetPriority(cfg->regs, channel, config->channel_priority);
	}

	/* Initialize channel */
	dma_status = Cy_DMA_Channel_Init(cfg->regs, channel, &channel_config);
	if (dma_status != CY_DMA_SUCCESS) {
		return -EIO;
	}

	irq_unlock(key);
	return 0;
}

DW_Type *ifx_cat1_dma_get_regs(const struct device *dev)
{
	const struct ifx_cat1_dma_config *const cfg = dev->config;

	return cfg->regs;
}

static int ifx_cat1_dma_start(const struct device *dev, uint32_t channel)
{
	const struct ifx_cat1_dma_config *const cfg = dev->config;
	struct ifx_cat1_dma_data *data = dev->data;

	if (channel >= cfg->num_channels) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	/* Enable DMA interrupt source. */
	Cy_DMA_Channel_SetInterruptMask(cfg->regs, channel, CY_DMA_INTR_MASK);

	/* Enable the interrupt  */
	irq_enable(data->channels[channel].irq);

	/* Enable DMA channel */
	Cy_DMA_Channel_Enable(cfg->regs, channel);
	if ((data->channels[channel].channel_direction == MEMORY_TO_MEMORY) ||
	    (data->channels[channel].channel_direction == MEMORY_TO_PERIPHERAL)) {
		cyhal_dma_t dma_obj = {
			.resource.type = CYHAL_RSC_DW,
			.resource.block_num = _get_hw_block_num(cfg->regs),
			.resource.channel_num = channel,
		};
		(void)cyhal_dma_start_transfer(&dma_obj);
	}
	return 0;
}

static int ifx_cat1_dma_stop(const struct device *dev, uint32_t channel)
{
	const struct ifx_cat1_dma_config *const cfg = dev->config;

	if (channel >= cfg->num_channels) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	/* Disable DMA channel */
	Cy_DMA_Channel_Disable(cfg->regs, channel);

	return 0;
}

int ifx_cat1_dma_reload(const struct device *dev, uint32_t channel, uint32_t src, uint32_t dst,
			size_t size)
{
	struct ifx_cat1_dma_data *data = dev->data;
	const struct ifx_cat1_dma_config *const cfg = dev->config;
	cy_stc_dma_descriptor_t *descriptor = data->channels[channel].descr;

	if (channel >= cfg->num_channels) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	/* Disable Channel */
	Cy_DMA_Channel_Disable(cfg->regs, channel);

	/* Update source/destination address for the specified descriptor */
	descriptor->src = (uint32_t)IFX_CAT1_DMA_SRC_ADDR(src);
	descriptor->dst = dst;

	/* Initialize channel */
	Cy_DMA_Channel_Enable(cfg->regs, channel);

	return 0;
}

uint32_t get_total_size(const struct device *dev, uint32_t channel)
{
	struct ifx_cat1_dma_data *data = dev->data;
	uint32_t total_size = 0;
	uint32_t x_size = 0;
	uint32_t y_size = 0;
	cy_stc_dma_descriptor_t *curr_descr = data->channels[channel].descr;

	while (curr_descr != NULL) {
		x_size = Cy_DMA_Descriptor_GetXloopDataCount(curr_descr);

		if (CY_DMA_2D_TRANSFER == Cy_DMA_Descriptor_GetDescriptorType(curr_descr)) {
			y_size = Cy_DMA_Descriptor_GetYloopDataCount(curr_descr);
		} else {
			y_size = 0;
		}

		total_size += y_size != 0 ? x_size * y_size : x_size;
		curr_descr = Cy_DMA_Descriptor_GetNextDescriptor(curr_descr);
	}

	return total_size;
}

uint32_t get_transferred_size(const struct device *dev, uint32_t channel)
{
	struct ifx_cat1_dma_data *data = dev->data;
	const struct ifx_cat1_dma_config *const cfg = dev->config;
	uint32_t transferred_data_size = 0;
	uint32_t x_size = 0;
	uint32_t y_size = 0;

	cy_stc_dma_descriptor_t *next_descr = data->channels[channel].descr;
	cy_stc_dma_descriptor_t *curr_descr =
		Cy_DMA_Channel_GetCurrentDescriptor(ifx_cat1_dma_get_regs(dev), channel);

	/* Calculates all processed descriptors */
	while ((next_descr != NULL) && (next_descr != curr_descr)) {
		x_size = Cy_DMA_Descriptor_GetXloopDataCount(next_descr);
		y_size = Cy_DMA_Descriptor_GetYloopDataCount(next_descr);
		transferred_data_size += y_size != 0 ? x_size * y_size : x_size;
		next_descr = Cy_DMA_Descriptor_GetNextDescriptor(next_descr);
	}

	/* Calculates current descriptors (in progress) */
	transferred_data_size +=
		_FLD2VAL(DW_CH_STRUCT_CH_IDX_X_IDX, DW_CH_IDX(cfg->regs, channel)) +
		(_FLD2VAL(DW_CH_STRUCT_CH_IDX_Y_IDX, DW_CH_IDX(cfg->regs, channel)) *
		 Cy_DMA_Descriptor_GetXloopDataCount(curr_descr));

	return transferred_data_size;
}

static int ifx_cat1_dma_get_status(const struct device *dev, uint32_t channel,
				   struct dma_status *stat)
{
	struct ifx_cat1_dma_data *data = dev->data;
	const struct ifx_cat1_dma_config *const cfg = dev->config;
	uint32_t pending_status = 0;

	if (channel >= cfg->num_channels) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	if (stat != NULL) {
		/* Check is current DMA channel busy or idle */
#if CONFIG_SOC_FAMILY_INFINEON_CAT1A
		pending_status = DW_CH_STATUS(cfg->regs, channel) &
				 (1UL << DW_CH_STRUCT_V2_CH_STATUS_PENDING_Pos);
#elif CONFIG_SOC_FAMILY_INFINEON_CAT1B
		pending_status = DW_CH_STATUS(cfg->regs, channel) &
				 (1UL << DW_CH_STRUCT_CH_STATUS_PENDING_Pos);
#endif
		/* busy status info */
		stat->busy = pending_status ? true : false;

		if (data->channels[channel].descr != NULL) {
			uint32_t total_transfer_size = get_total_size(dev, channel);
			uint32_t transferred_size = get_transferred_size(dev, channel);

			stat->pending_length = total_transfer_size - transferred_size;
		} else {
			stat->pending_length = 0;
		}

		/* direction info */
		stat->dir = data->channels[channel].channel_direction;
	}

	return 0;
}

#if CYHAL_DRIVER_AVAILABLE_SYSPM && CONFIG_PM

static bool _cyhal_dma_dmac_pm_callback(cyhal_syspm_callback_state_t state,
					cyhal_syspm_callback_mode_t mode, void *callback_arg)
{
	CY_UNUSED_PARAMETER(state);
	bool block_transition = false;
	struct ifx_cat1_dma_config *conf = (struct ifx_cat1_dma_config *)callback_arg;
	uint8_t i;

	switch (mode) {
	case CYHAL_SYSPM_CHECK_READY:
		for (i = 0u; i < conf->num_channels; i++) {
#if CONFIG_SOC_FAMILY_INFINEON_CAT1A
			block_transition |= DW_CH_STATUS(conf->regs, i) &
					    (1UL << DW_CH_STRUCT_V2_CH_STATUS_PENDING_Pos);
#elif CONFIG_SOC_FAMILY_INFINEON_CAT1B
			block_transition |= DW_CH_STATUS(conf->regs, i) &
					    (1UL << DW_CH_STRUCT_CH_STATUS_PENDING_Pos);
#endif
		}
		break;
	case CYHAL_SYSPM_CHECK_FAIL:
	case CYHAL_SYSPM_AFTER_TRANSITION:
		break;
	default:
		CY_ASSERT(false);
		break;
	}

	return !block_transition;
}
#endif

static int ifx_cat1_dma_init(const struct device *dev)
{
	const struct ifx_cat1_dma_config *const cfg = dev->config;

#if CYHAL_DRIVER_AVAILABLE_SYSPM && CONFIG_PM
	struct ifx_cat1_dma_data *data = dev->data;

	_cyhal_syspm_register_peripheral_callback(&data->syspm_callback_args);
#endif

	/* Enable DMA block to start descriptor execution process */
	Cy_DMA_Enable(cfg->regs);

	/* Configure IRQ */
	cfg->irq_configure();

	return 0;
}

/* Handles DMA interrupts and dispatches to the individual channel */
struct ifx_cat1_dma_irq_context {
	const struct device *dev;
	uint32_t channel;
};

static void ifx_cat1_dma_isr(struct ifx_cat1_dma_irq_context *irq_context)
{
	uint32_t channel = irq_context->channel;
	struct ifx_cat1_dma_data *data = irq_context->dev->data;
	const struct ifx_cat1_dma_config *cfg = irq_context->dev->config;
	dma_callback_t callback = data->channels[channel].callback;
	int status;

	/* Remove all allocated linked descriptors */
	_dma_free_linked_descriptors(data->channels[channel].descr);
	data->channels[channel].descr = NULL;

	uint32_t intr_status = Cy_DMA_Channel_GetStatus(cfg->regs, channel);

	/* Clear all interrupts */
	Cy_DMA_Channel_ClearInterrupt(cfg->regs, channel);

	/* Get interrupt type and call users event callback if they have enabled that event */
	switch (intr_status) {
	case CY_DMA_INTR_CAUSE_COMPLETION:
		status = 0;
		break;
	case CY_DMA_INTR_CAUSE_DESCR_BUS_ERROR: /* Descriptor bus error */
	case CY_DMA_INTR_CAUSE_SRC_BUS_ERROR:   /* Source bus error */
	case CY_DMA_INTR_CAUSE_DST_BUS_ERROR:   /* Destination bus error */
		status = -EPERM;
		break;
	case CY_DMA_INTR_CAUSE_SRC_MISAL: /* Source address is not aligned */
	case CY_DMA_INTR_CAUSE_DST_MISAL: /* Destination address is not aligned */
		status = -EPERM;
		break;
	case CY_DMA_INTR_CAUSE_CURR_PTR_NULL:      /* Current descr pointer is NULL */
	case CY_DMA_INTR_CAUSE_ACTIVE_CH_DISABLED: /* Active channel is disabled */
	default:
		status = -EIO;
		break;
	}

	if ((callback != NULL && status == 0) ||
	    (callback != NULL && data->channels[channel].error_callback_dis)) {
		void *callback_arg = data->channels[channel].user_data;

		callback(irq_context->dev, callback_arg, channel, status);
	}
}

static DEVICE_API(dma, ifx_cat1_dma_api) = {
	.config = ifx_cat1_dma_configure,
	.start = ifx_cat1_dma_start,
	.stop = ifx_cat1_dma_stop,
	.reload = ifx_cat1_dma_reload,
	.get_status = ifx_cat1_dma_get_status,
};

#define IRQ_CONFIGURE(n, inst)                                                                     \
	static const struct ifx_cat1_dma_irq_context irq_context##inst##n = {                      \
		.dev = DEVICE_DT_INST_GET(inst),                                                   \
		.channel = n,                                                                      \
	};                                                                                         \
                                                                                                   \
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(inst, n, irq), DT_INST_IRQ_BY_IDX(inst, n, priority),       \
		    ifx_cat1_dma_isr, &irq_context##inst##n, 0);                                   \
                                                                                                   \
	ifx_cat1_dma_channels##inst[n].irq = DT_INST_IRQ_BY_IDX(inst, n, irq);

#define CONFIGURE_ALL_IRQS(inst, n) LISTIFY(n, IRQ_CONFIGURE, (), inst)

#if CYHAL_DRIVER_AVAILABLE_SYSPM && CONFIG_PM
#define SYSPM_CALLBACK_ARGS(n)                                                                     \
	.syspm_callback_args = {                                                                   \
		.callback = &_cyhal_dma_dmac_pm_callback,                                          \
		.states = (cyhal_syspm_callback_state_t)(CYHAL_SYSPM_CB_CPU_DEEPSLEEP |            \
							 CYHAL_SYSPM_CB_CPU_DEEPSLEEP_RAM |        \
							 CYHAL_SYSPM_CB_SYSTEM_HIBERNATE),         \
		.next = NULL,                                                                      \
		.args = (void *)&ifx_cat1_dma_config##n,                                           \
		.ignore_modes =                                                                    \
			(cyhal_syspm_callback_mode_t)(CYHAL_SYSPM_BEFORE_TRANSITION |              \
						      CYHAL_SYSPM_AFTER_DS_WFI_TRANSITION)},
#else
#define SYSPM_CALLBACK_ARGS(n)
#endif

#define INFINEON_CAT1_DMA_INIT(n)                                                                  \
                                                                                                   \
	static void ifx_cat1_dma_irq_configure##n(void);                                           \
                                                                                                   \
                                                                                                   \
	static struct ifx_cat1_dma_channel                                                         \
		ifx_cat1_dma_channels##n[DT_INST_PROP(n, dma_channels)];                           \
                                                                                                   \
	static const struct ifx_cat1_dma_config ifx_cat1_dma_config##n = {                         \
		.num_channels = DT_INST_PROP(n, dma_channels),                                     \
		.regs = (DW_Type *)DT_INST_REG_ADDR(n),                                            \
		.irq_configure = ifx_cat1_dma_irq_configure##n,                                    \
	};                                                                                         \
                                                                                                   \
	ATOMIC_DEFINE(ifx_cat1_dma_##n, DT_INST_PROP(n, dma_channels));                            \
	static __aligned(32) struct ifx_cat1_dma_data ifx_cat1_dma_data##n = {                     \
		.ctx =                                                                             \
			{                                                                          \
				.magic = DMA_MAGIC,                                                \
				.atomic = ifx_cat1_dma_##n,                                        \
				.dma_channels = DT_INST_PROP(n, dma_channels),                     \
			},                                                                         \
		.channels = ifx_cat1_dma_channels##n,                                              \
		SYSPM_CALLBACK_ARGS(n)};                                                           \
                                                                                                   \
	static void ifx_cat1_dma_irq_configure##n(void)                                            \
	{                                                                                          \
		extern struct ifx_cat1_dma_channel ifx_cat1_dma_channels##n[];                     \
		CONFIGURE_ALL_IRQS(n, DT_NUM_IRQS(DT_DRV_INST(n)));                                \
	}                                                                                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &ifx_cat1_dma_init, NULL, &ifx_cat1_dma_data##n,                  \
			      &ifx_cat1_dma_config##n, PRE_KERNEL_1, CONFIG_DMA_INIT_PRIORITY,     \
			      &ifx_cat1_dma_api);

DT_INST_FOREACH_STATUS_OKAY(INFINEON_CAT1_DMA_INIT)
