/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/dma.h>
#include <zephyr/logging/log.h>
#include <stdint.h>
#include "soc.h"
#include "r_dmac.h"

LOG_MODULE_REGISTER(dma_renesas_ra, CONFIG_DMA_LOG_LEVEL);

#define DT_DRV_COMPAT renesas_ra_dma

extern void dmac_int_isr(void);

struct ra_dma_channel_config {
	int irq;
	int ipl;
};

struct ra_dma_config {
	void (*irq_configure)(void);
	const struct device *clock_dev;
	const struct ra_dma_channel_config *const channels;
	const struct clock_control_ra_subsys_cfg clock_subsys;
	const uint32_t channel_count;
};

struct ra_dma_channel_context {
	const struct device *dev;
	uint32_t channel;
};

struct ra_dma_channel_data {
	dmac_instance_ctrl_t fsp_ctrl;
	transfer_cfg_t fsp_cfg;
	transfer_info_t fsp_info;
	dmac_extended_cfg_t fsp_extend;
	struct ra_dma_channel_context context;
	struct dma_config config;
};

struct ra_dma_data {
	struct dma_context context;
	const struct ra_dma_config *const config;
	struct ra_dma_channel_data *const channels;
};

static void dma_renesas_ra_callback_handler(dmac_callback_args_t *args)
{
	struct ra_dma_channel_context *context = (struct ra_dma_channel_context *)args->p_context;

	const uint32_t channel = context->channel;
	const struct device *dev = context->dev;
	struct ra_dma_data *data = dev->data;
	dma_callback_t user_cb = data->channels[channel].config.dma_callback;
	void *user_data = data->channels[channel].config.user_data;

	if (user_cb) {
		user_cb(dev, user_data, channel, DMA_STATUS_COMPLETE);
	}
}

static bool dma_renesas_ra_channel_is_valid(const struct device *dev, uint32_t channel)
{
	struct ra_dma_data *data = dev->data;

	/* Check if the channel index is within the valid range */
	if (channel >= data->config->channel_count) {
		return false;
	}

	/* Check if the channel has a valid interrupt vector and is not disabled */
	if (data->config->channels[channel].irq == FSP_INVALID_VECTOR ||
	    data->config->channels[channel].ipl == BSP_IRQ_DISABLED) {
		return false;
	}

	return true;
}

static bool dma_renesas_ra_config_is_valid(struct dma_config *config)
{
	/* Check for null configuration, null block, or zero block count */
	if (config == NULL || config->head_block == NULL || config->block_count == 0) {
		return false;
	}

	/* Ensure source and destination data sizes are the same */
	if (config->source_data_size != config->dest_data_size) {
		return false;
	}

	/* Block size must be a multiple of source data size */
	if (config->head_block->block_size % config->source_data_size != 0) {
		return false;
	}

	/* Source address must be aligned to the source data size */
	if (config->head_block->source_address % config->source_data_size != 0) {
		return false;
	}

	/* Destination address must be aligned to the destination data size */
	if (config->head_block->dest_address % config->dest_data_size != 0) {
		return false;
	}

	return true;
}

static bool dma_renesas_ra_config_is_support(struct dma_config *config)
{
	/* Currently only supports MEMORY_TO_MEMORY channel direction */
	if (config->channel_direction != MEMORY_TO_MEMORY) {
		return false;
	}

	/* Cyclic mode is not supported */
	if (config->cyclic) {
		return false;
	}

	/* Source or destination handshake is not supported */
	if (config->source_handshake || config->dest_handshake) {
		return false;
	}

	/* Channel chaining is not supported */
	if (config->source_chaining_en || config->dest_chaining_en) {
		return false;
	}

	/* Source gather or destination scatter is not supported */
	if (config->head_block->source_gather_en || config->head_block->dest_scatter_en) {
		return false;
	}

	/* Only single block transfer is supported */
	if (config->block_count > 1) {
		return false;
	}

	return true;
}

static int dma_renesas_ra_config_prepare(const struct device *dev, uint32_t channel,
					 struct dma_config *config)
{
	struct ra_dma_data *data = dev->data;
	struct ra_dma_channel_data *ch = &data->channels[channel];
	uint32_t transfers_count = (config->head_block->block_size / config->source_data_size);

	/* Set source address adjustment mode */
	switch (config->head_block->source_addr_adj) {
	case DMA_ADDR_ADJ_INCREMENT:
		ch->fsp_info.transfer_settings_word_b.src_addr_mode =
			TRANSFER_ADDR_MODE_INCREMENTED;
		break;
	case DMA_ADDR_ADJ_DECREMENT:
		ch->fsp_info.transfer_settings_word_b.src_addr_mode =
			TRANSFER_ADDR_MODE_DECREMENTED;
		break;
	case DMA_ADDR_ADJ_NO_CHANGE:
		ch->fsp_info.transfer_settings_word_b.src_addr_mode = TRANSFER_ADDR_MODE_FIXED;
		break;
	default:
		return -EINVAL;
	}

	/* Set destination address adjustment mode */
	switch (config->head_block->dest_addr_adj) {
	case DMA_ADDR_ADJ_INCREMENT:
		ch->fsp_info.transfer_settings_word_b.dest_addr_mode =
			TRANSFER_ADDR_MODE_INCREMENTED;
		break;
	case DMA_ADDR_ADJ_DECREMENT:
		ch->fsp_info.transfer_settings_word_b.dest_addr_mode =
			TRANSFER_ADDR_MODE_DECREMENTED;
		break;
	case DMA_ADDR_ADJ_NO_CHANGE:
		ch->fsp_info.transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_FIXED;
		break;
	default:
		return -EINVAL;
	}

	/* Set source and destination addresses */
	ch->fsp_info.p_src = (void *)config->head_block->source_address;
	ch->fsp_info.p_dest = (void *)config->head_block->dest_address;

	/* Set data size for each transfer */
	switch (config->source_data_size) {
	case 1U:
		ch->fsp_info.transfer_settings_word_b.size = TRANSFER_SIZE_1_BYTE;
		break;
	case 2U:
		ch->fsp_info.transfer_settings_word_b.size = TRANSFER_SIZE_2_BYTE;
		break;
	case 4U:
		ch->fsp_info.transfer_settings_word_b.size = TRANSFER_SIZE_4_BYTE;
		break;
	default:
		return -EINVAL;
	}

	/* Set transfer mode for DMA transfer */
	ch->fsp_info.transfer_settings_word_b.mode = TRANSFER_MODE_NORMAL;

	/* Set block count and length for DMA transfer */
	ch->fsp_info.num_blocks = (uint16_t)config->block_count;

	/* Transfer count must not exceed hardware limit */
	if (transfers_count > UINT16_MAX) {
		return -EINVAL;
	}
	ch->fsp_info.length = (uint16_t)transfers_count;

	/* Set interrupt to trigger at the end of the transfer */
	ch->fsp_info.transfer_settings_word_b.irq = TRANSFER_IRQ_END;

	/* Set trigger source to software trigger */
	ch->fsp_extend.activation_source = ELC_EVENT_NONE;

	/* Initialize context for DMA callbacks */
	ch->context.channel = channel;
	ch->context.dev = dev;

	/* Set up remaining fields in the FSP extension structure */
	ch->fsp_extend.p_context = &ch->context;
	ch->fsp_extend.p_callback = dma_renesas_ra_callback_handler;
	ch->fsp_extend.channel = (uint8_t)channel;
	ch->fsp_extend.irq = (IRQn_Type)data->config->channels[channel].irq;
	ch->fsp_extend.ipl = (IRQn_Type)data->config->channels[channel].ipl;

	/* Link transfer info and extension structure to the FSP configuration */
	ch->fsp_cfg.p_info = &ch->fsp_info;
	ch->fsp_cfg.p_extend = &ch->fsp_extend;

	/* Save DMA configuration to channel context */
	memcpy(&ch->config, config, sizeof(struct dma_config));

	return 0;
}

static int dma_renesas_ra_config(const struct device *dev, uint32_t channel,
				 struct dma_config *config)
{
	struct ra_dma_data *data = dev->data;
	struct ra_dma_channel_data *ch = &data->channels[channel];
	fsp_err_t err;
	int ret;

	/* Validate the DMA channel */
	if (!dma_renesas_ra_channel_is_valid(dev, channel)) {
		LOG_DBG("Invalid DMA channel: %d.", channel);
		return -EINVAL;
	}

	/* Validate the provided DMA configuration */
	if (!dma_renesas_ra_config_is_valid(config)) {
		LOG_DBG("Invalid DMA config for channel %d.", channel);
		return -EINVAL;
	}

	/* Check if the configuration is supported by the driver */
	if (!dma_renesas_ra_config_is_support(config)) {
		LOG_DBG("Unsupported DMA config for channel %d.", channel);
		return -ENOTSUP;
	}

	/* Prepare internal structures and hardware settings for the DMA transfer */
	ret = dma_renesas_ra_config_prepare(dev, channel, config);
	if (ret) {
		LOG_DBG("Failed to prepare DMA config for channel %d.", channel);
		return ret;
	}

	/* Open the DMA channel or reconfigure if already open */
	if (ch->fsp_ctrl.open) {
		err = R_DMAC_Reconfigure(&ch->fsp_ctrl, &ch->fsp_info);
		if (err != FSP_SUCCESS) {
			LOG_DBG("Failed to reconfigure DMA channel %d.", channel);
			return -EIO;
		}
	} else {
		err = R_DMAC_Open(&ch->fsp_ctrl, &ch->fsp_cfg);
		if (err != FSP_SUCCESS) {
			LOG_DBG("Failed to open DMA channel %d.", channel);
			return -EIO;
		}
	}

	return 0;
}

static int dma_renesas_ra_reload(const struct device *dev, uint32_t channel, uint32_t src,
				 uint32_t dst, size_t size)
{
	struct ra_dma_data *data = dev->data;
	struct ra_dma_channel_data *ch = &data->channels[channel];
	uint32_t data_size = ch->config.source_data_size;
	fsp_err_t err;

	/* Validate the DMA channel */
	if (!dma_renesas_ra_channel_is_valid(dev, channel)) {
		LOG_DBG("DMA channel %d is invalid.", channel);
		return -EINVAL;
	}

	/* Check if the DMA channel is open */
	if (ch->fsp_ctrl.open == 0) {
		LOG_DBG("DMA channel %d is not configured.", channel);
		return -EINVAL;
	}

	/* Validate the transfer size */
	if (size == 0 || size % data_size != 0) {
		LOG_DBG("DMA transfer size is invalid.");
		return -EINVAL;
	}

	/* Reload  DMA controller with new source, destination, and size */
	err = R_DMAC_Reset(&ch->fsp_ctrl, (void *)src, (void *)dst, (uint16_t)(size / data_size));
	if (err != FSP_SUCCESS) {
		LOG_DBG("DMA channel %d reload failed: 0x%x", channel, err);
		return -EIO;
	}

	return 0;
}

static int dma_renesas_ra_start(const struct device *dev, uint32_t channel)
{
	struct ra_dma_data *data = dev->data;
	struct ra_dma_channel_data *ch = &data->channels[channel];
	fsp_err_t err;

	/* Validate the DMA channel */
	if (!dma_renesas_ra_channel_is_valid(dev, channel)) {
		LOG_DBG("DMA channel %d is invalid.", channel);
		return -EINVAL;
	}

	/* Check if the DMA channel is open */
	if (ch->fsp_ctrl.open == 0) {
		LOG_DBG("DMA channel %d is not configured.", channel);
		return -EINVAL;
	}

	/* Enable the DMA channel */
	err = R_DMAC_Enable(&ch->fsp_ctrl);
	if (err != FSP_SUCCESS) {
		LOG_DBG("DMA channel %d enable failed: 0x%x", channel, err);
		return -EIO;
	}

	/* Start the DMA transfer using software trigger */
	err = R_DMAC_SoftwareStart(&ch->fsp_ctrl, TRANSFER_START_MODE_REPEAT);
	if (err != FSP_SUCCESS) {
		LOG_DBG("DMA channel %d start failed: 0x%x", channel, err);
		return -EIO;
	}

	return 0;
}

static int dma_renesas_ra_stop(const struct device *dev, uint32_t channel)
{
	struct ra_dma_data *data = dev->data;
	struct ra_dma_channel_data *ch = &data->channels[channel];
	fsp_err_t err;

	/* Validate the DMA channel */
	if (!dma_renesas_ra_channel_is_valid(dev, channel)) {
		LOG_DBG("DMA channel %d is invalid.", channel);
		return -EINVAL;
	}

	/* Check if the DMA channel is open */
	if (ch->fsp_ctrl.open == 0) {
		LOG_DBG("DMA channel %d is not configured.", channel);
		return -EINVAL;
	}

	/* Issue a software stop to halt the DMA transfer */
	err = R_DMAC_SoftwareStop(&ch->fsp_ctrl);
	if (err != FSP_SUCCESS) {
		LOG_DBG("DMA channel %d stop failed: 0x%x", channel, err);
		return -EIO;
	}

	/* Disable the DMA channel */
	err = R_DMAC_Disable(&ch->fsp_ctrl);
	if (err != FSP_SUCCESS) {
		LOG_DBG("DMA channel %d disable failed: 0x%x", channel, err);
		return -EIO;
	}

	return 0;
}

static int dma_renesas_ra_get_status(const struct device *dev, uint32_t channel,
				     struct dma_status *status)
{
	struct ra_dma_data *data = dev->data;
	struct ra_dma_channel_data *ch = &data->channels[channel];
	uint32_t data_size = ch->config.source_data_size;
	transfer_properties_t info;
	fsp_err_t err;

	/* Validate the DMA channel */
	if (!dma_renesas_ra_channel_is_valid(dev, channel)) {
		LOG_DBG("DMA channel %d is invalid.", channel);
		return -EINVAL;
	}

	/* Check if the DMA channel is open */
	if (ch->fsp_ctrl.open == 0) {
		LOG_DBG("DMA channel %d is not configured.", channel);
		return -EINVAL;
	}

	/* Retrieve current transfer information */
	err = R_DMAC_InfoGet(&ch->fsp_ctrl, &info);
	if (err != FSP_SUCCESS) {
		LOG_DBG("DMA channel %d get info failed: 0x%x", channel, err);
		return -EIO;
	}

	/* Initialize status structure to zero */
	memset(status, 0, sizeof(struct dma_status));

	/* Set transfer direction */
	status->dir = ch->config.channel_direction;

	/* Calculate remaining bytes to transfer */
	status->pending_length = info.transfer_length_remaining * data_size;

	/* Indicate whether the DMA is busy */
	status->busy = status->pending_length ? true : false;

	/* Calculate total bytes copied so far */
	status->total_copied = (ch->fsp_info.length - info.transfer_length_remaining) * data_size;

	return 0;
}

static bool dma_renesas_ra_chan_filter(const struct device *dev, int channel, void *filter_param)
{
	ARG_UNUSED(filter_param);

	struct ra_dma_data *data = dev->data;
	struct ra_dma_channel_data *ch = &data->channels[channel];

	/* Check if the DMA channel is valid */
	if (!dma_renesas_ra_channel_is_valid(dev, channel)) {
		return false;
	}

	/* Check if the channel is already open */
	if (ch->fsp_ctrl.open != 0) {
		return false;
	}

	return true;
}

static void dma_renesas_ra_chan_release(const struct device *dev, uint32_t channel)
{
	struct ra_dma_data *data = dev->data;
	struct ra_dma_channel_data *ch = &data->channels[channel];

	/* Check if the DMA channel is valid */
	if (!dma_renesas_ra_channel_is_valid(dev, channel)) {
		return;
	}

	/* Close the DMA channel to release resources */
	R_DMAC_Close(&ch->fsp_ctrl);
}

static DEVICE_API(dma, dma_renesas_ra_driver_api) = {
	.config = dma_renesas_ra_config,
	.reload = dma_renesas_ra_reload,
	.start = dma_renesas_ra_start,
	.stop = dma_renesas_ra_stop,
	.get_status = dma_renesas_ra_get_status,
	.chan_filter = dma_renesas_ra_chan_filter,
	.chan_release = dma_renesas_ra_chan_release,
};

static int dma_renesas_ra_init(const struct device *dev)
{
	struct ra_dma_data *const data = dev->data;
	int ret;

	/* Check if the DMA controller clock device is ready */
	if (!device_is_ready(data->config->clock_dev)) {
		return -ENODEV;
	}

	/* Enable the DMA peripheral clock */
	ret = clock_control_on(data->config->clock_dev,
			       (clock_control_subsys_t)&data->config->clock_subsys);
	if (ret < 0) {
		return ret;
	}

	/* Configure DMA-related interrupts */
	data->config->irq_configure();

	return 0;
}

/* Macro for DMA channel interrupt event code */
#define EVENT_DMA_CH_INT(n, inst) BSP_PRV_IELS_ENUM(CONCAT(EVENT_DMAC, n, _INT))

/* Macro to get DMA channel IRQ or use default if not defined */
#define DMA_CH_IRQ_BY_NAME_OR(n, inst, cell, default)                                              \
	COND_CODE_1(DT_INST_IRQ_HAS_NAME(inst, ch##n),                                             \
		   (DT_INST_IRQ_BY_NAME(inst, ch##n, cell)),                                       \
		   (default))

/* Macro to define DMA channel configuration struct initializer */
#define DMA_CH_CONFIG(n, inst)                                                                     \
	{                                                                                          \
		.irq = DMA_CH_IRQ_BY_NAME_OR(n, inst, irq, FSP_INVALID_VECTOR),                    \
		.ipl = DMA_CH_IRQ_BY_NAME_OR(n, inst, priority, BSP_IRQ_DISABLED),                 \
	}

/* Macro to create a list of DMA channel configuration initializers */
#define DMA_CH_CONFIG_LIST(inst) LISTIFY(DT_INST_PROP(inst, dma_channels), DMA_CH_CONFIG, (,), inst)

/* Macro to configure IRQ for a DMA channel if defined in DTS */
#define DMA_CH_IRQ_CONFIG(n, inst)                                                                 \
	COND_CODE_1(DT_INST_IRQ_HAS_NAME(inst, ch##n),                                             \
		(R_ICU->IELSR[DT_INST_IRQ_BY_NAME(inst, ch##n, irq)] =                             \
			EVENT_DMA_CH_INT(n, inst);                                                 \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, ch##n, irq),                                 \
		DT_INST_IRQ_BY_NAME(inst, ch##n, priority), dmac_int_isr, NULL, 0);), ())

/* Macro to configure IRQs for all DMA channels in a list */
#define DMA_CH_IRQ_CONFIG_LIST(inst)                                                               \
	LISTIFY(DT_INST_PROP(inst, dma_channels), DMA_CH_IRQ_CONFIG, (;), inst)

/* Macro for DMA driver initialization */
#define DMA_RA_INIT(inst)                                                                          \
	static void dma_renesas_ra_irq_configure##inst(void)                                       \
	{                                                                                          \
		DMA_CH_IRQ_CONFIG_LIST(inst);                                                      \
	}                                                                                          \
                                                                                                   \
	static const struct ra_dma_channel_config ra_dma_channel_config##inst[DT_INST_PROP(        \
		inst, dma_channels)] = {DMA_CH_CONFIG_LIST(inst)};                                 \
                                                                                                   \
	static const struct ra_dma_config ra_dma_config##inst = {                                  \
		.irq_configure = dma_renesas_ra_irq_configure##inst,                               \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                             \
		.clock_subsys =                                                                    \
			{                                                                          \
				.mstp = (uint32_t)DT_INST_CLOCKS_CELL_BY_IDX(inst, 0, mstp),       \
				.stop_bit = DT_INST_CLOCKS_CELL_BY_IDX(inst, 0, stop_bit),         \
			},                                                                         \
		.channels = ra_dma_channel_config##inst,                                           \
		.channel_count = DT_INST_PROP(inst, dma_channels),                                 \
	};                                                                                         \
                                                                                                   \
	static struct ra_dma_channel_data                                                          \
		ra_dma_channel_data##inst[DT_INST_PROP(inst, dma_channels)] = {0};                 \
                                                                                                   \
	ATOMIC_DEFINE(dma_renesas_ra_atomic##inst, DT_INST_PROP(inst, dma_channels));              \
                                                                                                   \
	static struct ra_dma_data ra_dma_data##inst = {                                            \
		.context =                                                                         \
			{                                                                          \
				.magic = DMA_MAGIC,                                                \
				.atomic = dma_renesas_ra_atomic##inst,                             \
				.dma_channels = DT_INST_PROP(inst, dma_channels),                  \
			},                                                                         \
		.channels = ra_dma_channel_data##inst,                                             \
		.config = &ra_dma_config##inst,                                                    \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, dma_renesas_ra_init, NULL, &ra_dma_data##inst,                 \
			      &ra_dma_config##inst, PRE_KERNEL_1, CONFIG_DMA_INIT_PRIORITY,        \
			      &dma_renesas_ra_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DMA_RA_INIT)
