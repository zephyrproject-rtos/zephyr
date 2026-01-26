/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief DMA driver for Infineon CAT1 MCU family.
 */

#define DT_DRV_COMPAT infineon_dma

#include <infineon_kconfig.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/cache.h>

#include <cy_pdl.h>
#include <soc.h>
LOG_MODULE_REGISTER(ifx_cat1_dma, CONFIG_DMA_LOG_LEVEL);

#ifndef CY_REMAP_ADDRESS_CBUS_TO_SAHB
#define CY_REMAP_ADDRESS_CBUS_TO_SAHB(addr) (addr)
#endif

struct ifx_cat1_dma_channel {
	uint32_t channel_direction: 3;
	uint32_t complete_callback_en: 1;
	uint32_t error_callback_dis: 1;

	cy_stc_dma_descriptor_t descr;
	IRQn_Type irq;

	/* store config data from dma_config structure */
	dma_callback_t callback;
	void *user_data;
};

struct ifx_cat1_dma_data {
	struct ifx_cat1_dma_channel *channels;
	cy_stc_dma_descriptor_t descriptor_pool[CONFIG_INFINEON_DESCRIPTOR_POOL_SIZE];
	ATOMIC_DEFINE(desc_allocated, CONFIG_INFINEON_DESCRIPTOR_POOL_SIZE);
};

struct ifx_cat1_dma_config {
	DW_Type *regs;
	void (*irq_configure)(void);
	bool enable_chaining;
	uint8_t num_channels;
};

static cy_stc_dma_descriptor_t *ifx_cat1_dma_alloc_descriptor(const struct device *dev)
{
	uint32_t i;
	struct ifx_cat1_dma_data *data = dev->data;

	for (i = 0u; i < CONFIG_INFINEON_DESCRIPTOR_POOL_SIZE; i++) {
		if (!atomic_test_and_set_bit(data->desc_allocated, i)) {
			return &data->descriptor_pool[i];
		}
	}

	return NULL;
}

int ifx_cat1_dma_trig(const struct device *dev, uint32_t channel)
{
	const struct ifx_cat1_dma_config *const cfg = dev->config;
	struct ifx_cat1_dma_data *data = dev->data;

	/* In general, we do SW trigger in the beginning if src comes from memory.
	 * The reason is if src comes from peripheral, trigger signal from peripheral
	 * will trigger DMA in the beginning.
	 */
	if ((data->channels[channel].channel_direction == MEMORY_TO_MEMORY) ||
	    (data->channels[channel].channel_direction == MEMORY_TO_PERIPHERAL)) {
		/* Set SW trigger for the channel */
		Cy_DMA_Channel_SetSWTrigger(cfg->regs, channel);
	}

	return 0;
}

static cy_en_dma_data_size_t convert_dma_data_size_z_to_pdl(struct dma_config *config)
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

static int convert_dma_xy_increment_z_to_pdl(uint32_t addr_adj)
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

/* Configure a channel */
static int ifx_cat1_dma_config(const struct device *dev, uint32_t channel,
			       struct dma_config *config)
{
	cy_en_dma_status_t dma_status;
	struct ifx_cat1_dma_data *data = dev->data;
	const struct ifx_cat1_dma_config *const cfg = dev->config;
	cy_stc_dma_channel_config_t channel_config = {0u};
	cy_stc_dma_descriptor_config_t descriptor_config = {0u};
	cy_stc_dma_descriptor_t *descriptor = NULL;

	if (channel >= cfg->num_channels) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	/* Support only the same data width for source and dest */
	if (config->dest_data_size != config->source_data_size) {
		LOG_ERR("Source and dest data size differ.");
		return -EINVAL;
	}

	/* Support only the same burst_length for source and dest */
	if (config->dest_burst_length != config->source_burst_length) {
		LOG_ERR("Source and dest burst_length differ.");
		return -EINVAL;
	}

	/* DataWire only supports <=256 byte burst and <=256 bytes per burst */
	if ((config->dest_burst_length > 256) ||
	    (config->dest_burst_length <= 1 && config->head_block->block_size > 256) ||
	    (config->dest_burst_length > 0 &&
	     config->head_block->block_size > (config->dest_burst_length * 256))) {
		LOG_ERR("DMA (DW) only supports <=256 byte burst and <=256 bytes per burst");
		return -EINVAL;
	}

	if (config->dest_data_size != 1 && config->dest_data_size != 2 &&
	    config->dest_data_size != 4) {
		LOG_ERR("dest_data_size must be 1, 2, or 4 (%" PRIu32 ")", config->dest_data_size);
		return -EINVAL;
	}

	/* Update callback configuration while we have the lock - ISR reads these fields */
	data->channels[channel].callback = config->dma_callback;
	data->channels[channel].user_data = config->user_data;
	data->channels[channel].channel_direction = config->channel_direction;
	data->channels[channel].complete_callback_en = config->complete_callback_en;
	data->channels[channel].error_callback_dis = config->error_callback_dis;

	/* Get first descriptor */
	descriptor = &data->channels[channel].descr;

	/* Retrigger descriptor immediately */
	descriptor_config.retrigger = CY_DMA_RETRIG_IM;

	/* Setup Interrupt Type
	 * if config->complete_callback_en = 0, callback invoked at completion only.
	 * if config->complete_callback_en = 1, callback invoked at
	 * completion of each block.
	 */
	if (!config->complete_callback_en) {
		descriptor_config.interruptType = CY_DMA_DESCR_CHAIN;
	} else {
		descriptor_config.interruptType = CY_DMA_DESCR;
	}

	if (config->block_count > 1U) {
		descriptor_config.channelState = CY_DMA_CHANNEL_ENABLED;
	} else {
		descriptor_config.channelState = CY_DMA_CHANNEL_DISABLED;
	}

	descriptor_config.triggerOutType = CY_DMA_DESCR_CHAIN;

	if (config->channel_direction == MEMORY_TO_MEMORY) {
		descriptor_config.triggerInType = CY_DMA_DESCR_CHAIN;
	} else {
		descriptor_config.triggerInType = CY_DMA_1ELEMENT;
	}

	/* Set data size byte / 2 bytes / word */
	descriptor_config.dataSize = convert_dma_data_size_z_to_pdl(config);

	/* By default, transfer what the user set for dataSize. However,
	 * if transferring between memory and a peripheral, make sure the
	 * peripheral access is using words.
	 */
	descriptor_config.srcTransferSize = CY_DMA_TRANSFER_SIZE_DATA;
	descriptor_config.dstTransferSize = CY_DMA_TRANSFER_SIZE_DATA;

	if (config->channel_direction == PERIPHERAL_TO_MEMORY) {
		descriptor_config.srcTransferSize = CY_DMA_TRANSFER_SIZE_WORD;
	} else if (config->channel_direction == MEMORY_TO_PERIPHERAL) {
		descriptor_config.dstTransferSize = CY_DMA_TRANSFER_SIZE_WORD;
	}

	struct dma_block_config *block_config = config->head_block;

	for (uint32_t i = 0u; i < config->block_count; i++) {

		/* Setup destination increment for X source loop */
		descriptor_config.srcXincrement =
			convert_dma_xy_increment_z_to_pdl(block_config->source_addr_adj);

		/* Setup destination increment for X destination loop */
		descriptor_config.dstXincrement =
			convert_dma_xy_increment_z_to_pdl(block_config->dest_addr_adj);

		/* Calculate total number of data elements in this block */
		uint32_t total_elements = block_config->block_size / config->dest_data_size;

		/* Setup 1D/2D descriptor for each data block */
		if (config->dest_burst_length != 0) {
			descriptor_config.descriptorType = CY_DMA_2D_TRANSFER;
			descriptor_config.xCount = config->dest_burst_length;
			descriptor_config.yCount =
				DIV_ROUND_UP(total_elements, config->dest_burst_length);
			descriptor_config.srcYincrement =
				descriptor_config.srcXincrement * config->dest_burst_length;
			descriptor_config.dstYincrement =
				descriptor_config.dstXincrement * config->dest_burst_length;
		} else {
			descriptor_config.descriptorType = CY_DMA_1D_TRANSFER;
			descriptor_config.xCount = total_elements;
			descriptor_config.yCount = 1;
			descriptor_config.srcYincrement = 0;
			descriptor_config.dstYincrement = 0;
		}

		/* Set source and destination for descriptor
		 * Note: In devices with CBUS and SAHB address spaces, the DMA only supports SAHB
		 * mapped transactions.
		 */
		descriptor_config.srcAddress =
			(void *)CY_REMAP_ADDRESS_CBUS_TO_SAHB((void *)block_config->source_address);
		descriptor_config.dstAddress =
			(void *)CY_REMAP_ADDRESS_CBUS_TO_SAHB((void *)block_config->dest_address);

		/* Allocate next descriptor if need */
		if (i + 1u < config->block_count) {
			descriptor_config.nextDescriptor = ifx_cat1_dma_alloc_descriptor(dev);
			if (descriptor_config.nextDescriptor == NULL) {
				LOG_ERR("ERROR: can not allocate DMA descriptor");
			}
		} else {
			if (cfg->enable_chaining) {
				descriptor_config.nextDescriptor = descriptor;
			} else {
				descriptor_config.nextDescriptor = NULL;
			}
		}

		/* initialize descriptor */
		dma_status = Cy_DMA_Descriptor_Init(descriptor, &descriptor_config);
		if (dma_status != CY_DMA_SUCCESS) {
			return -EIO;
		}

		block_config = block_config->next_block;
		descriptor = descriptor_config.nextDescriptor;
	}

	/* Set a descriptor for the specified DMA channel */
	channel_config.descriptor = &data->channels[channel].descr;

	/* Set a priority for the DMA channel */
	Cy_DMA_Channel_SetPriority(cfg->regs, channel, config->channel_priority);

	/* Initialize channel */
	dma_status = Cy_DMA_Channel_Init(cfg->regs, channel, &channel_config);
	if (dma_status != CY_DMA_SUCCESS) {
		return -EIO;
	}

	/* Enable DMA interrupt source. */
	Cy_DMA_Channel_SetInterruptMask(cfg->regs, channel, CY_DMA_INTR_MASK);

	/* Enable the interrupt  */
	irq_enable(data->channels[channel].irq);

	return 0;
}

static int ifx_cat1_dma_start(const struct device *dev, uint32_t channel)
{
	const struct ifx_cat1_dma_config *const cfg = dev->config;

	if (channel >= cfg->num_channels) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	/* Flush the cache before starting DMA to ensure that the modifications made in cache
	 * are written back to the memory.
	 */
#if defined(CONFIG_CPU_HAS_DCACHE) && defined(__DCACHE_PRESENT) && __DCACHE_PRESENT
	sys_cache_data_flush_and_invd_all();
#endif

	/* Enable DMA channel */
	Cy_DMA_Channel_Enable(cfg->regs, channel);

	ifx_cat1_dma_trig(dev, channel);

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
	cy_stc_dma_descriptor_t *descriptor = &data->channels[channel].descr;

	if (channel >= cfg->num_channels) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	/* Set a descriptor for the specified DMA channel
	 *
	 * Note: In devices with CBUS and SAHB address spaces, the DMA only supports SAHB
	 * mapped transactions.
	 */
	descriptor->src = (uint32_t)CY_REMAP_ADDRESS_CBUS_TO_SAHB((void *)src);
	descriptor->dst = (uint32_t)CY_REMAP_ADDRESS_CBUS_TO_SAHB((void *)dst);

	/* Flush the cache before starting DMA to ensure that the modifications made in cache
	 * are written back to the memory.
	 */
#ifdef CONFIG_CPU_HAS_DCACHE
	sys_cache_data_flush_and_invd_range((void *)src, size);
#endif

	/* Initialize channel */
	Cy_DMA_Channel_Enable(cfg->regs, channel);

	return 0;
}

static uint32_t get_total_size(const struct device *dev, uint32_t channel)
{
	struct ifx_cat1_dma_data *data = dev->data;
	const struct ifx_cat1_dma_config *const cfg = dev->config;
	uint32_t total_size = 0;
	cy_stc_dma_descriptor_t *curr_descr;
	uint32_t x_size = 0;
	uint32_t y_size = 0;

	if (channel >= cfg->num_channels) {
		return 0;
	}

	/* start from the head descriptor for the channel */
	curr_descr = &data->channels[channel].descr;

	while (curr_descr != NULL) {
		x_size = Cy_DMA_Descriptor_GetXloopDataCount(curr_descr);
		if (Cy_DMA_Descriptor_GetDescriptorType(curr_descr) == CY_DMA_2D_TRANSFER) {
			y_size = Cy_DMA_Descriptor_GetYloopDataCount(curr_descr);
		} else {
			y_size = 0;
		}
		total_size += (y_size != 0) ? (x_size * y_size) : x_size;
		curr_descr = Cy_DMA_Descriptor_GetNextDescriptor(curr_descr);
	}

	return total_size;
}

static uint32_t get_transferred_size(const struct device *dev, uint32_t channel)
{
	const struct ifx_cat1_dma_config *const cfg = dev->config;
	struct ifx_cat1_dma_data *data = dev->data;
	uint32_t transferred_data_size = 0;
	uint32_t x_size = 0;
	uint32_t y_size = 0;

	/* head descriptor for the channel */
	cy_stc_dma_descriptor_t *next_descr = &data->channels[channel].descr;
	/* current descriptor from PDL */
	cy_stc_dma_descriptor_t *curr_descr =
		Cy_DMA_Channel_GetCurrentDescriptor(cfg->regs, channel);

	/* Sanity */
	if (next_descr == NULL || curr_descr == NULL) {
		return 0;
	}

	/* Count fully processed descriptors */
	while ((next_descr != NULL) && (next_descr != curr_descr)) {
		x_size = Cy_DMA_Descriptor_GetXloopDataCount(next_descr);
		y_size = Cy_DMA_Descriptor_GetYloopDataCount(next_descr);
		transferred_data_size += (y_size != 0U) ? (x_size * y_size) : x_size;
		next_descr = Cy_DMA_Descriptor_GetNextDescriptor(next_descr);
	}

	/* Add progress inside the current descriptor using DW INDEX registers */
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

	if (channel >= cfg->num_channels) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	if (stat == NULL) {
		return -EINVAL;
	}

	/* Use PDL to determine whether channel is active: current descriptor is non-NULL when
	 * channel is executing.
	 */
	stat->busy = DW_CH_STATUS(cfg->regs, channel) & (1UL << DW_CH_STRUCT_CH_STATUS_PENDING_Pos)
			     ? true
			     : false;

	/* Check if the channel has a configured head descriptor by inspecting its src/dst */
	cy_stc_dma_descriptor_t *head = &data->channels[channel].descr;

	if (head != NULL && (head->src != 0 || head->dst != 0)) {
		uint32_t total_transfer_size = get_total_size(dev, channel);
		uint32_t transferred_size = get_transferred_size(dev, channel);

		/* pending_length expressed in user units (items), same as get_total/get_transferred
		 */
		stat->pending_length = total_transfer_size - transferred_size;
	} else {
		stat->pending_length = 0;
	}

	/* direction info */
	stat->dir = data->channels[channel].channel_direction;

	return 0;
}

static int ifx_cat1_dma_init(const struct device *dev)
{
	const struct ifx_cat1_dma_config *const cfg = dev->config;

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
	int status = -EIO;

	/* Get interrupt type and call users event callback if they have enabled that event */
	uint32_t intr_cause = Cy_DMA_Channel_GetStatus(cfg->regs, channel);

	switch (intr_cause) {
	case CY_DMA_INTR_CAUSE_COMPLETION:
		status = 0;
		break;
	case CY_DMA_INTR_CAUSE_DESCR_BUS_ERROR: /* Descriptor bus error   */
		LOG_ERR("DMA error: Descriptor bus error (cause=0x%x)", intr_cause);
		status = -EIO;
		break;
	case CY_DMA_INTR_CAUSE_SRC_BUS_ERROR: /* Source bus error       */
		LOG_ERR("DMA error: Source bus error (cause=0x%x)", intr_cause);
		status = -EIO;
		break;
	case CY_DMA_INTR_CAUSE_DST_BUS_ERROR: /* Destination bus error  */
		LOG_ERR("DMA error: Destination bus error (cause=0x%x)", intr_cause);
		status = -EIO;
		break;
	case CY_DMA_INTR_CAUSE_SRC_MISAL: /* Source address is not aligned      */
		LOG_ERR("DMA error: Source misaligned (cause=0x%x)", intr_cause);
		status = -EIO;
		break;
	case CY_DMA_INTR_CAUSE_DST_MISAL: /* Destination address is not aligned */
		LOG_ERR("DMA error: Destination misaligned (cause=0x%x)", intr_cause);
		status = -EIO;
		break;
	case CY_DMA_INTR_CAUSE_CURR_PTR_NULL: /* Current descr pointer is NULL   */
		LOG_ERR("DMA error: Current descriptor pointer is NULL (cause=0x%x)", intr_cause);
		status = -EIO;
		break;
	case CY_DMA_INTR_CAUSE_ACTIVE_CH_DISABLED: /* Active channel is disabled      */
		LOG_ERR("DMA error: Active channel disabled (cause=0x%x)", intr_cause);
		status = -EIO;
		break;
	default:
		LOG_WRN("DMA unknown interrupt cause: 0x%x", intr_cause);
		status = -EIO;
		break;
	}

	/* Clear all interrupts */
	Cy_DMA_Channel_ClearInterrupt(cfg->regs, channel);

	/* return if callback is not registered */
	if (callback == NULL) {
		return;
	}

	/* Give callback with error status only if enabled */
	if (status != 0 && !data->channels[channel].error_callback_dis) {
		return;
	}

	callback(irq_context->dev, data->channels[channel].user_data, channel, status);
}

static DEVICE_API(dma, ifx_cat1_dma_api) = {
	.config = ifx_cat1_dma_config,
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

#define INFINEON_CAT1_DMA_INIT(n)                                                                  \
                                                                                                   \
	static void ifx_cat1_dma_irq_configure##n(void);                                           \
                                                                                                   \
	static struct ifx_cat1_dma_channel                                                         \
		ifx_cat1_dma_channels##n[DT_INST_PROP(n, dma_channels)];                           \
                                                                                                   \
	static __aligned(32) struct ifx_cat1_dma_data ifx_cat1_dma_data##n = {                     \
		.channels = ifx_cat1_dma_channels##n,                                              \
	};                                                                                         \
                                                                                                   \
	static const struct ifx_cat1_dma_config ifx_cat1_dma_config##n = {                         \
		.regs = (DW_Type *)DT_INST_REG_ADDR(n),                                            \
		.irq_configure = ifx_cat1_dma_irq_configure##n,                                    \
		.enable_chaining = DT_INST_PROP(n, enable_chaining),                               \
		.num_channels = DT_INST_PROP(n, dma_channels),                                     \
	};                                                                                         \
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
