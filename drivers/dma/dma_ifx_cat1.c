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

#include <zephyr/devicetree.h>
LOG_MODULE_REGISTER(ifx_cat1_dma, CONFIG_DMA_LOG_LEVEL);

#define DMA_CH_NUM           29
#define DESCRIPTOR_POOL_SIZE 10 /* TODO: add parameter to Kconfig */

struct ifx_cat1_dma_channel {
	uint32_t channel_direction: 3;
	uint32_t complete_callback_en: 1;
	uint32_t error_callback_en: 1;

	cy_stc_dma_descriptor_t descr;
	IRQn_Type irq;

	/* store config data from dma_config structure */
	dma_callback_t callback;
	void *user_data;
};

struct ifx_cat1_dma_data {
	struct ifx_cat1_dma_channel *channels;
	cy_stc_dma_descriptor_t descriptor_pool[DESCRIPTOR_POOL_SIZE];
};

struct ifx_cat1_dma_config {
	DW_Type *regs;
	void (*irq_configure)(void);

	struct ifx_cat1_dma_channel_dt_info *channels_dt_info;
	uint8_t channels_dt_count;
};

struct ifx_cat1_dma_channel_dt_info {
	uint8_t channel_id;
	cy_stc_dma_channel_config_t config;
	cy_stc_dma_descriptor_config_t *descr;
	uint8_t descr_num;
};

static int32_t _get_hw_block_num(DW_Type *reg_addr)
{
#if (CPUSS_DW0_PRESENT == 1)
	if ((uint32_t) reg_addr == DW0_BASE) {
		return 0;
	}
#endif

#if (CPUSS_DW1_PRESENT == 1)
	if ((uint32_t) reg_addr == DW1_BASE) {
		return 1;
	}
#endif
	return 0;
}

static cy_stc_dma_descriptor_t *ifx_cat1_dma_alloc_descriptor(const struct device *dev)
{
	uint32_t i;
	struct ifx_cat1_dma_data *data = dev->data;

	for (i = 0u; i < DESCRIPTOR_POOL_SIZE; i++) {
		if (data->descriptor_pool[i].src == 0) {
			data->descriptor_pool[i].src = 0xFF;
			return &data->descriptor_pool[i];
		}
	}

	return NULL;
}

void ifx_cat1_dma_free_descriptor(cy_stc_dma_descriptor_t *descr)
{
	if (descr != NULL) {
		memset(descr, 0, sizeof(cy_stc_dma_descriptor_t));
	}
}

static struct ifx_cat1_dma_channel_dt_info *ifx_cat1_get_config_from_dt(const struct device *dev,
									uint32_t channel)
{
	const struct ifx_cat1_dma_config *const cfg = dev->config;
	uint32_t i;

	if (cfg->channels_dt_info != NULL) {
		for (i = 0u; i < cfg->channels_dt_count; i++) {
			if (cfg->channels_dt_info[i].channel_id == channel) {
				return &cfg->channels_dt_info[i];
			}
		}
	}
	return NULL;
}

int ifx_cat1_dma_ex_connect_digital(const struct device *dev, uint32_t channel,
				    cyhal_source_t source, cyhal_dma_input_t input)
{
	struct ifx_cat1_dma_data *data = dev->data;
	const struct ifx_cat1_dma_config *const cfg = dev->config;

	cyhal_dma_t dma_obj = {
		.resource.type = CYHAL_RSC_DW,
		.resource.block_num = _get_hw_block_num(cfg->regs),
		.resource.channel_num = channel,
		.descriptor.dw = data->channels[channel].descr,
	};

	cy_rslt_t rslt = cyhal_dma_connect_digital(&dma_obj, source, input);

	return rslt ? -EIO : 0;
}

int ifx_cat1_dma_ex_enable_output(const struct device *dev, uint32_t channel,
				  cyhal_dma_output_t output, cyhal_source_t *source)
{
	struct ifx_cat1_dma_data *data = dev->data;
	const struct ifx_cat1_dma_config *const cfg = dev->config;

	cyhal_dma_t dma_obj = {
		.resource.type = CYHAL_RSC_DW,
		.resource.block_num = _get_hw_block_num(cfg->regs),
		.resource.channel_num = channel,
		.descriptor.dw = data->channels[channel].descr,
	};

	cy_rslt_t rslt = cyhal_dma_enable_output(&dma_obj, output, source);

	return rslt ? -EIO : 0;
}

int ifx_cat1_dma_trig(const struct device *dev, uint32_t channel)
{
	struct ifx_cat1_dma_data *data = dev->data;
	const struct ifx_cat1_dma_config *const cfg = dev->config;

	cyhal_dma_t dma_obj = {
		.resource.type = CYHAL_RSC_DW,
		.resource.block_num = _get_hw_block_num(cfg->regs),
		.resource.channel_num = channel,
		.descriptor.dw = data->channels[channel].descr,
	};

	cy_rslt_t rslt = cyhal_dma_start_transfer(&dma_obj);

	return rslt ? -EIO : 0;
}

cy_en_dma_data_size_t _convert_dma_data_size_z_to_pdl(struct dma_config *config)
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

int _convert_dma_xy_increment_z_to_pdl(uint32_t addr_adj)
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
	bool use_dt_config = false;
	cy_en_dma_status_t dma_status;
	struct ifx_cat1_dma_data *data = dev->data;
	const struct ifx_cat1_dma_config *const cfg = dev->config;

	cy_stc_dma_channel_config_t channel_config = {0u};
	cy_stc_dma_descriptor_config_t descriptor_config = {0u};
	cy_stc_dma_descriptor_t *descriptor = NULL;

	if (channel >= DMA_CH_NUM) {
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

	data->channels[channel].callback = config->dma_callback;
	data->channels[channel].user_data = config->user_data;
	data->channels[channel].channel_direction = config->channel_direction;
	data->channels[channel].complete_callback_en = config->complete_callback_en;
	data->channels[channel].error_callback_en = config->error_callback_en;

	/* Lock and page in the channel configuration */
	uint32_t key = irq_lock();

	/* Get first descriptor */
	descriptor = &data->channels[channel].descr;

	/* If we have Descriptor configuration in DT for this channel,
	 * so use it and skip configuration from dma_config.
	 */
	struct ifx_cat1_dma_channel_dt_info *dma_channel_dt_info =
		ifx_cat1_get_config_from_dt(dev, channel);

	if (dma_channel_dt_info != NULL) {
		/* Use configuration from device tree */
		uint32_t descr_index = (uint32_t)dma_channel_dt_info->config.descriptor;

		/* Get channel configuration */
		channel_config = dma_channel_dt_info->config;

		/* Get descriptor configuration */
		descriptor_config = dma_channel_dt_info->descr[descr_index];

		/* Set source and destination for descriptor */
		descriptor_config.srcAddress = (void *)config->head_block->source_address;
		descriptor_config.dstAddress = (void *)config->head_block->dest_address;

		/* Initialize descriptor */
		dma_status = Cy_DMA_Descriptor_Init(descriptor, &descriptor_config);
		if (dma_status != CY_DMA_SUCCESS) {
			return -EIO;
		}

		use_dt_config = true;
	} else {

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

		/* Keep CHANNEL_ENABLED if BURST transfer (config->dest_burst_length != 0) */
		if (config->dest_burst_length != 0) {
			descriptor_config.channelState = CY_DMA_CHANNEL_ENABLED;
		} else {
			descriptor_config.channelState = CY_DMA_CHANNEL_DISABLED;
		}

		/* TODO: should be able to configure. */
		descriptor_config.triggerOutType = CY_DMA_DESCR_CHAIN;
		descriptor_config.triggerInType = CY_DMA_DESCR_CHAIN;

		/* Set data size byte / 2 bytes / word */
		descriptor_config.dataSize = _convert_dma_data_size_z_to_pdl(config);

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
				_convert_dma_xy_increment_z_to_pdl(block_config->source_addr_adj);

			/* Setup destination increment for X destination loop */
			descriptor_config.dstXincrement =
				_convert_dma_xy_increment_z_to_pdl(block_config->dest_addr_adj);

			/* Setup 1D/2D descriptor for each data block */
			if (config->dest_burst_length != 0) {
				descriptor_config.descriptorType = CY_DMA_2D_TRANSFER;
				descriptor_config.xCount = config->dest_burst_length;
				descriptor_config.yCount = DIV_ROUND_UP(block_config->block_size,
									config->dest_burst_length);
				descriptor_config.srcYincrement =
					descriptor_config.srcXincrement * config->dest_burst_length;
				descriptor_config.dstYincrement =
					descriptor_config.dstXincrement * config->dest_burst_length;
			} else {
				descriptor_config.descriptorType = CY_DMA_1D_TRANSFER;
				descriptor_config.xCount = block_config->block_size;
				descriptor_config.yCount = 1;
				descriptor_config.srcYincrement = 0;
				descriptor_config.dstYincrement = 0;
			}

			/* Set source and destination for descriptor */
			descriptor_config.srcAddress = (void *)block_config->source_address;
			descriptor_config.dstAddress = (void *)block_config->dest_address;

			/* Allocate next descriptor if need */
			descriptor_config.nextDescriptor = ifx_cat1_dma_alloc_descriptor(dev);

			if (i + 1u < config->block_count) {
				if (descriptor_config.nextDescriptor == NULL) {
					LOG_ERR("ERROR: can not allocate DMA descriptor");
				}
			} else {
				descriptor_config.nextDescriptor = NULL;
			}

			/* initialize descriptor */
			dma_status = Cy_DMA_Descriptor_Init(descriptor, &descriptor_config);
			if (dma_status != CY_DMA_SUCCESS) {
				return -EIO;
			}

			block_config = block_config->next_block;
			descriptor = descriptor_config.nextDescriptor;
		}
	}

	/* Set a descriptor for the specified DMA channel */
	channel_config.descriptor = &data->channels[channel].descr;

	/* Set a priority for the DMA channel */
	if (use_dt_config == false) {
		Cy_DMA_Channel_SetPriority(cfg->regs, channel, config->channel_priority);
	}

	/* Initialize channel */
	dma_status = Cy_DMA_Channel_Init(cfg->regs, channel, &channel_config);
	if (dma_status != CY_DMA_SUCCESS) {
		return -EIO;
	}

	/* Enable DMA interrupt source. */
	Cy_DMA_Channel_SetInterruptMask(cfg->regs, channel, CY_DMA_INTR_MASK);

	/* Enable the interrupt  */
	irq_enable(data->channels[channel].irq);

	Cy_DMA_Channel_Enable(cfg->regs, channel);

	irq_unlock(key);
	return 0;
}

static int ifx_cat1_dma_start(const struct device *dev, uint32_t channel)
{
	const struct ifx_cat1_dma_config *const cfg = dev->config;

	if (channel >= DMA_CH_NUM) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	/* Enable DMA channel */
	Cy_DMA_Channel_Enable(cfg->regs, channel);

	ifx_cat1_dma_trig(dev, channel);

	return 0;
}

static int ifx_cat1_dma_stop(const struct device *dev, uint32_t channel)
{
	const struct ifx_cat1_dma_config *const cfg = dev->config;

	if (channel >= DMA_CH_NUM) {
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
	/* NOTE: reload is not supported now */

	if (channel >= DMA_CH_NUM) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	return 0;
}

static int ifx_cat1_dma_get_status(const struct device *dev, uint32_t channel,
				   struct dma_status *stat)
{
	struct ifx_cat1_dma_data *data = dev->data;
	const struct ifx_cat1_dma_config *const cfg = dev->config;

	if (channel >= DMA_CH_NUM) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	if (stat != NULL) {
		/* Check is current DMA channel busy or idle */
		uint32_t active_ch = Cy_DMA_GetActiveChannel(cfg->regs);

		/* busy status info */
		stat->busy = (active_ch & (1ul << channel)) ? true : false;

		/* direction info */
		stat->dir = data->channels[channel].channel_direction;
	}

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
	int status;

	/* Get interrupt type and call users event callback if they have enabled that event */
	switch (Cy_DMA_Channel_GetStatus(cfg->regs, channel)) {
	case CY_DMA_INTR_CAUSE_COMPLETION:
		status = 0;
		break;
	case CY_DMA_INTR_CAUSE_DESCR_BUS_ERROR: /* Descriptor bus error   */
	case CY_DMA_INTR_CAUSE_SRC_BUS_ERROR:   /* Source bus error       */
	case CY_DMA_INTR_CAUSE_DST_BUS_ERROR:   /* Destination bus error  */
		status = -EPERM;
		break;
	case CY_DMA_INTR_CAUSE_SRC_MISAL: /* Source address is not aligned      */
	case CY_DMA_INTR_CAUSE_DST_MISAL: /* Destination address is not aligned */
		status = -EPERM;
		break;
	case CY_DMA_INTR_CAUSE_CURR_PTR_NULL:      /* Current descr pointer is NULL   */
	case CY_DMA_INTR_CAUSE_ACTIVE_CH_DISABLED: /* Active channel is disabled      */
	default:
		status = -EIO;
		break;
	}

	if ((callback != NULL && status == 0) ||
	    (callback != NULL && data->channels[channel].error_callback_en)) {
		void *callback_arg = data->channels[channel].user_data;

		callback(irq_context->dev, callback_arg, channel, status);
	}

	/* Clear all interrupts */
	Cy_DMA_Channel_ClearInterrupt(cfg->regs, channel);
}

static const struct dma_driver_api ifx_cat1_dma_api = {
	.config = ifx_cat1_dma_config,
	.start = ifx_cat1_dma_start,
	.stop = ifx_cat1_dma_stop,
	/*.reload = ifx_cat1_dma_reload, NOTE: .reload is not support now */
	.get_status = ifx_cat1_dma_get_status,
};

#define NODE_CHILD_IS_EMPTY(node_id) IS_EMPTY(DT_SUPPORTS_DEP_ORDS(node_id))

/* DMA descriptors configuration */
#define DESCRIPTOR_CONFIG(id)                                                                      \
	{                                                                                          \
		.retrigger = DT_ENUM_IDX_OR(id, retrigger, CY_DMA_RETRIG_IM),                      \
		.interruptType = DT_ENUM_IDX_OR(id, interrupt_type, CY_DMA_DESCR),                 \
		.triggerOutType = DT_ENUM_IDX_OR(id, trigger_out_type, CY_DMA_DESCR),              \
		.triggerInType = DT_ENUM_IDX_OR(id, trigger_in_type, CY_DMA_DESCR),                \
		.channelState = DT_ENUM_IDX_OR(id, channel_state, CY_DMA_CHANNEL_DISABLED),        \
                                                                                                   \
		.dataSize = DT_ENUM_IDX_OR(id, data_size, CY_DMA_BYTE),                            \
		.srcTransferSize =                                                                 \
			DT_ENUM_IDX_OR(id, src_transfer_size, CY_DMA_TRANSFER_SIZE_DATA),          \
		.dstTransferSize =                                                                 \
			DT_ENUM_IDX_OR(id, dst_transfer_size, CY_DMA_TRANSFER_SIZE_DATA),          \
                                                                                                   \
		.descriptorType = DT_ENUM_IDX(id, descriptor_type),                                \
                                                                                                   \
		.srcXincrement = DT_PROP_OR(id, src_x_increment, 0),                               \
		.dstXincrement = DT_PROP_OR(id, dst_x_increment, 0),                               \
		.xCount = DT_PROP_OR(id, x_count, 0),                                              \
                                                                                                   \
		.srcYincrement = DT_PROP_OR(id, src_y_increment, 0),                               \
		.dstYincrement = DT_PROP_OR(id, src_y_increment, 0),                               \
		.yCount = DT_PROP_OR(id, y_count, 0),                                              \
	},

/* DMA channel configuration */
#define CHANNELS_CONFIG(chan_id)                                                                   \
	{.channel_id = DT_REG_ADDR(chan_id),                                                       \
	 .config = {.priority = DT_PROP_OR(chan_id, priority, 3),                                  \
		    .preemptable = DT_PROP_OR(chan_id, preemptable, 0),                            \
		    .descriptor = DT_PROP_OR(chan_id, descriptor, 0)},                             \
	 COND_CODE_1(DT_NODE_EXISTS(DT_CHILD(chan_id, descriptor_0)),                              \
		     (.descr = (cy_stc_dma_descriptor_config_t *)&descriptor_conf_##chan_id,       \
		      .descr_num = sizeof(descriptor_conf_##chan_id) /                             \
				   sizeof(cy_stc_dma_descriptor_config_t),),                       \
		     (.descr = NULL, .descr_num = 0,))},

#define DESCRIPTOR_FOREACH_CHILD_FUNC(chan_id)                                                     \
	COND_CODE_1(DT_NODE_EXISTS(DT_CHILD(chan_id, descriptor_0)),                               \
		    (static const cy_stc_dma_descriptor_config_t descriptor_conf_##chan_id[] =     \
			     {DT_CAT(chan_id, _FOREACH_CHILD)(DESCRIPTOR_CONFIG)};),               \
		    (EMPTY))

#define GET_CHANNEL_DT_INFO(inst)								   \
	DT_INST_FOREACH_CHILD(inst, DESCRIPTOR_FOREACH_CHILD_FUNC)                                 \
	static const struct ifx_cat1_dma_channel_dt_info ifx_cat1_dma_channel_dt_info_##inst[] = { \
		DT_INST_FOREACH_CHILD(inst, CHANNELS_CONFIG)};


#define IRQ_CONFIGURE(n, inst)                                                                     \
	static const struct ifx_cat1_dma_irq_context irq_context##inst##n = {                      \
		.dev = DEVICE_DT_INST_GET(inst),                                                   \
		.channel = n,                                                                      \
	};                                                                                         \
                                                                                                   \
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(inst, n, irq), DT_INST_IRQ_BY_IDX(inst, n, priority),       \
		    ifx_cat1_dma_isr, &irq_context##inst##n, 0);                                   \
                                                                                                   \
	ifx_cat1_dma_channels##inst[n].irq = DT_INST_IRQ(inst, irq);

#define CONFIGURE_ALL_IRQS(inst, n) LISTIFY(n, IRQ_CONFIGURE, (), inst)

#define INFINEON_CAT1_DMA_INIT(n)                                                                  \
                                                                                                   \
	static void ifx_cat1_dma_irq_configure##n(void);                                           \
												   \
	COND_CODE_0(NODE_CHILD_IS_EMPTY(DT_DRV_INST(n)),					   \
	(											   \
		GET_CHANNEL_DT_INFO(n);                                                            \
	), (EMPTY))										   \
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
                                                                                                   \
		COND_CODE_0(NODE_CHILD_IS_EMPTY(DT_DRV_INST(n)),                                   \
			    (									   \
				.channels_dt_info = (struct ifx_cat1_dma_channel_dt_info *)        \
							ifx_cat1_dma_channel_dt_info_##n,	   \
				.channels_dt_count = sizeof(ifx_cat1_dma_channel_dt_info_##n) /    \
						  sizeof(struct ifx_cat1_dma_channel_dt_info),),   \
			    (									   \
				.channels_dt_info = NULL,					   \
				.channels_dt_count = 0)						   \
			    )							                   \
                                                                                                   \
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
