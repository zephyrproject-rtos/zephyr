/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT renesas_rz_dma

#include <zephyr/drivers/dma.h>
#include "r_dmac_b.h"
#include <zephyr/logging/log.h>
#include "dma_renesas_rz.h"
LOG_MODULE_REGISTER(renesas_rz_dma);

/* FSP DMAC handler should be called within DMA ISR */
void dmac_b_int_isr(void);
void dmac_b_err_isr(void);

struct dmac_cb_ctx {
	const struct device *dmac_dev;
	uint32_t channel;
};

struct dma_channel_data {
	transfer_ctrl_t *fsp_ctrl;
	transfer_cfg_t fsp_cfg;

	/* INTID associated with the channel */
	int irq;
	int irq_ipl;

	/* DMA call back */
	dma_callback_t user_cb;
	void *user_data;
	struct dmac_cb_ctx cb_ctx;

	bool is_configured;
	uint32_t direction;
};

struct dma_renesas_rz_config {
	uint8_t unit;
	uint8_t num_channels;
	void (*irq_configure)(void);
	const transfer_api_t *fsp_api;
};

struct dma_renesas_rz_data {
	/* Dma context should be the first in data structure */
	struct dma_context ctx;
	struct dma_channel_data *channels;
};

static void dmac_rz_cb_handler(dmac_b_callback_args_t *args)
{
	struct dmac_cb_ctx *cb_ctx = (struct dmac_cb_ctx *)args->p_context;
	uint32_t channel = cb_ctx->channel;
	const struct device *dev = cb_ctx->dmac_dev;
	struct dma_renesas_rz_data *data = dev->data;
	dma_callback_t user_cb = data->channels[channel].user_cb;
	void *user_data = data->channels[channel].user_data;

	if (user_cb) {
		user_cb(dev, user_data, channel, args->event);
	}
}

static int dma_channel_common_checks(const struct device *dev, uint32_t channel)
{
	const struct dma_renesas_rz_config *config = dev->config;
	struct dma_renesas_rz_data *data = dev->data;

	if (channel >= config->num_channels) {
		LOG_ERR("%d: Invalid DMA channel %d.", __LINE__, channel);
		return -EINVAL;
	}

	if (!data->channels[channel].is_configured) {
		LOG_ERR("%d: DMA channel %d should first be configured.", __LINE__, channel);
		return -EINVAL;
	}

	return 0;
}

static inline int dma_channel_config_check_parameters(const struct device *dev,
						      struct dma_config *cfg)
{
	if ((cfg == NULL) || (cfg->head_block == NULL)) {
		LOG_ERR("%d: Missing configuration structure.", __LINE__);
		return -EFAULT;
	}

	if (1 < cfg->block_count) {
		LOG_ERR("%d: Link Mode is not supported, but only support 1 block per transfer",
			__LINE__);
		return -ENOTSUP;
	}

	if (cfg->source_chaining_en || cfg->dest_chaining_en) {
		LOG_ERR("%d:Channel Chainning is not supported.", __LINE__);
		return -ENOTSUP;
	}

	if (cfg->head_block->dest_scatter_count || cfg->head_block->source_gather_count ||
	    cfg->head_block->source_gather_interval || cfg->head_block->dest_scatter_interval) {
		LOG_ERR("%d: Scater and gather are not supported.", __LINE__);
		return -ENOTSUP;
	}

	return 0;
}

static int dma_channel_set_size(uint32_t size)
{
	int transfer_size;

	switch (size) {
	case 1:
		transfer_size = TRANSFER_SIZE_1_BYTE;
		break;
	case 2:
		transfer_size = TRANSFER_SIZE_2_BYTE;
		break;
	case 4:
		transfer_size = TRANSFER_SIZE_4_BYTE;
		break;
	case 8:
		transfer_size = TRANSFER_SIZE_8_BYTE;
		break;
	case 32:
		transfer_size = TRANSFER_SIZE_32_BYTE;
		break;
	case 64:
		transfer_size = TRANSFER_SIZE_64_BYTE;
		break;
	case 128:
		transfer_size = TRANSFER_SIZE_128_BYTE;
		break;
	default:
		LOG_ERR("%d: Unsupported data width.", __LINE__);
		return -ENOTSUP;
	}

	return transfer_size;
}

static inline int dma_channel_config_save_parameters(const struct device *dev, uint32_t channel,
						     struct dma_config *cfg)
{
	const struct dma_renesas_rz_config *config = dev->config;
	struct dma_renesas_rz_data *data = dev->data;
	transfer_info_t *p_info = data->channels[channel].fsp_cfg.p_info;
	dmac_b_extended_cfg_t *p_extend =
		(dmac_b_extended_cfg_t *)data->channels[channel].fsp_cfg.p_extend;

	memset(p_info, 0, sizeof(*p_info));
	memset(p_extend, 0, sizeof(*p_extend));

	/* Save transfer properties required by FSP */
	switch (cfg->head_block->dest_addr_adj) {
	case DMA_ADDR_ADJ_NO_CHANGE:
		p_info->dest_addr_mode = TRANSFER_ADDR_MODE_FIXED;
		break;
	case DMA_ADDR_ADJ_INCREMENT:
		p_info->dest_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED;
		break;
	default:
		LOG_ERR("%d, Unsupported destination address adjustemnt.", __LINE__);
		return -ENOTSUP;
	}

	switch (cfg->head_block->source_addr_adj) {
	case DMA_ADDR_ADJ_NO_CHANGE:
		p_info->src_addr_mode = TRANSFER_ADDR_MODE_FIXED;
		break;
	case DMA_ADDR_ADJ_INCREMENT:
		p_info->src_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED;
		break;
	default:
		LOG_ERR("%d, Unsupported source address adjustemnt.", __LINE__);
		return -ENOTSUP;
	}

	/* Convert data size following FSP convention */
	p_info->src_size = dma_channel_set_size(cfg->source_data_size);
	p_info->dest_size = dma_channel_set_size(cfg->dest_data_size);

	/* Save transfer properties required by FSP */
	p_info->p_src = (void const *volatile)cfg->head_block->source_address;
	p_info->p_dest = (void *volatile)cfg->head_block->dest_address;
	p_info->length = cfg->head_block->block_size;

	/*
	 * Properties of next 1 registers are assigned default value following FSP because transfer
	 * continuous is not supported.
	 */
	p_info->p_next1_src = NULL;
	p_info->p_next1_dest = NULL;
	p_info->next1_length = 1;
	p_extend->continuous_setting = DMAC_B_CONTINUOUS_SETTING_TRANSFER_ONCE;

	/* Save DMAC properties required by FSP */
	p_extend->unit = config->unit;
	p_extend->channel = channel;

	/* Save INTID and priority */
	p_extend->dmac_int_irq = data->channels[channel].irq;
	p_extend->dmac_int_ipl = data->channels[channel].irq_ipl;

	/* Save callback and data and use the generic handler. */
	data->channels[channel].user_cb = cfg->dma_callback;
	data->channels[channel].user_data = cfg->user_data;
	data->channels[channel].cb_ctx.dmac_dev = dev;
	data->channels[channel].cb_ctx.channel = channel;
	p_extend->p_callback = dmac_rz_cb_handler;
	p_extend->p_context = (void *)&data->channels[channel].cb_ctx;

	/* Save default value following FSP version */
	p_extend->ack_mode = DMAC_B_ACK_MODE_MASK_DACK_OUTPUT;
	p_extend->external_detection_mode = DMAC_B_EXTERNAL_DETECTION_NO_DETECTION;
	p_extend->internal_detection_mode = DMAC_B_INTERNAL_DETECTION_NO_DETECTION;

	/* Save properties with respect to a specific case */
	switch (cfg->channel_direction) {
	case MEMORY_TO_MEMORY:
		p_info->mode = TRANSFER_MODE_BLOCK;
		p_extend->activation_request_source_select =
			DMAC_B_REQUEST_DIRECTION_DESTINATION_MODULE;
		p_extend->activation_source = DMAC_TRIGGER_EVENT_SOFTWARE_TRIGGER;
		break;
	case PERIPHERAL_TO_MEMORY:
		p_info->mode = TRANSFER_MODE_NORMAL;
		p_extend->activation_request_source_select =
			DMAC_B_REQUEST_DIRECTION_DESTINATION_MODULE;
		p_extend->activation_source = cfg->dma_slot;
		break;
	case MEMORY_TO_PERIPHERAL:
		p_info->mode = TRANSFER_MODE_NORMAL;
		p_extend->activation_request_source_select = DMAC_B_REQUEST_DIRECTION_SOURCE_MODULE;
		p_extend->activation_source = cfg->dma_slot;
		break;
	default:
		LOG_ERR("%d: Unsupported direction mode.", __LINE__);
		return -ENOTSUP;
	}

	data->channels[channel].direction = cfg->channel_direction;

	/*
	 * Only support two priority modes, 0 is the highest priority with respect to FIXED
	 * Priority, Round Robin otherwise.
	 */
	if (cfg->channel_priority == 0) {
		p_extend->channel_scheduling = DMAC_B_CHANNEL_SCHEDULING_FIXED;
	} else {
		p_extend->channel_scheduling = DMAC_B_CHANNEL_SCHEDULING_ROUND_ROBIN;
	}

	if (1 < cfg->block_count) {
		LOG_ERR("%d: Link Mode is not supported.", __LINE__);
	} else {
		p_extend->dmac_mode = DMAC_B_MODE_SELECT_REGISTER;
	}

	return 0;
}

static int dma_renesas_rz_get_status(const struct device *dev, uint32_t channel,
				     struct dma_status *status)
{
	const struct dma_renesas_rz_config *config = dev->config;
	struct dma_renesas_rz_data *data = dev->data;

	int ret = dma_channel_common_checks(dev, channel);

	if (ret) {
		return ret;
	}

	transfer_info_t *p_info;

	p_info = data->channels[channel].fsp_cfg.p_info;
	transfer_properties_t properties;

	ret = config->fsp_api->infoGet(data->channels[channel].fsp_ctrl, &properties);
	if (FSP_SUCCESS != (fsp_err_t)ret) {
		LOG_ERR("%d: Failed to get info dma channel %d info with status %d.", __LINE__,
			channel, ret);
		return -EIO;
	}

	memset(status, 0, sizeof(*status));

	status->dir = data->channels[channel].direction;
	status->pending_length = properties.transfer_length_remaining;
	status->busy = status->pending_length ? true : false;
	status->total_copied = p_info->length - properties.transfer_length_remaining;

	return 0;
}

static int dma_renesas_rz_suspend(const struct device *dev, uint32_t channel)
{
	struct dma_renesas_rz_data *data = dev->data;

	int ret = dma_channel_common_checks(dev, channel);

	if (ret) {
		return ret;
	}

	dmac_b_instance_ctrl_t *p_ctrl = (dmac_b_instance_ctrl_t *)data->channels[channel].fsp_ctrl;

	uint8_t group = DMA_PRV_GROUP(channel);
	uint8_t prv_channel = DMA_PRV_CHANNEL(channel);

	/* Set transfer status is suspend */
	p_ctrl->p_reg->GRP[group].CH[prv_channel].CHCTRL = R_DMAC_B0_GRP_CH_CHCTRL_SETSUS_Msk;

	/* Check whether a transfer is suspended. */
	FSP_HARDWARE_REGISTER_WAIT(p_ctrl->p_reg->GRP[group].CH[prv_channel].CHSTAT_b.SUS, 1);

	return 0;
}

static int dma_renesas_rz_resume(const struct device *dev, uint32_t channel)
{
	struct dma_renesas_rz_data *data = dev->data;

	int ret = dma_channel_common_checks(dev, channel);

	if (ret) {
		return ret;
	}

	dmac_b_instance_ctrl_t *p_ctrl = (dmac_b_instance_ctrl_t *)data->channels[channel].fsp_ctrl;

	uint8_t group = DMA_PRV_GROUP(channel);
	uint8_t prv_channel = DMA_PRV_CHANNEL(channel);

	/* Check whether a transfer is suspended. */
	if (0 == p_ctrl->p_reg->GRP[group].CH[prv_channel].CHSTAT_b.SUS) {
		LOG_ERR("%d: DMA channel not suspend.", channel);
		return -EINVAL;
	}

	/* Restore transfer status from suspend */
	p_ctrl->p_reg->GRP[group].CH[prv_channel].CHCTRL |= R_DMAC_B0_GRP_CH_CHCTRL_CLRSUS_Msk;

	return 0;
}

static int dma_renesas_rz_stop(const struct device *dev, uint32_t channel)
{
	const struct dma_renesas_rz_config *config = dev->config;
	struct dma_renesas_rz_data *data = dev->data;

	int ret = dma_channel_common_checks(dev, channel);

	if (ret) {
		return ret;
	}

	ret = config->fsp_api->disable(data->channels[channel].fsp_ctrl);

	if (FSP_SUCCESS != (fsp_err_t)ret) {
		LOG_ERR("%d: Failed to stop dma channel %d info with status %d.", __LINE__, channel,
			ret);
		return -EIO;
	}

	return 0;
}

static int dma_renesas_rz_start(const struct device *dev, uint32_t channel)
{
	const struct dma_renesas_rz_config *config = dev->config;
	struct dma_renesas_rz_data *data = dev->data;
	dmac_b_extended_cfg_t const *p_extend;

	int ret = dma_channel_common_checks(dev, channel);

	if (ret) {
		return ret;
	}

	p_extend = data->channels[channel].fsp_cfg.p_extend;

	ret = config->fsp_api->enable(data->channels[channel].fsp_ctrl);

	if (FSP_SUCCESS != (fsp_err_t)ret) {
		LOG_ERR("%d: Failed to start %d dma channel with status %d.", __LINE__, channel,
			ret);
		return -EIO;
	}

	if (DMAC_TRIGGER_EVENT_SOFTWARE_TRIGGER == p_extend->activation_source) {
		ret = config->fsp_api->softwareStart(data->channels[channel].fsp_ctrl,
						     (transfer_start_mode_t)NULL);

		if (FSP_SUCCESS != (fsp_err_t)ret) {
			LOG_ERR("%d: Failed to trigger %d dma channel with status %d.", __LINE__,
				channel, ret);
			return -EIO;
		}
	}

	return 0;
}
static int dma_renesas_rz_config(const struct device *dev, uint32_t channel,
				 struct dma_config *dma_cfg)
{
	const struct dma_renesas_rz_config *config = dev->config;
	struct dma_renesas_rz_data *data = dev->data;
	struct dma_channel_data *channel_cfg;
	int ret;

	if (channel >= config->num_channels) {
		LOG_ERR("%d: Invalid DMA channel %d.", __LINE__, channel);
		return -EINVAL;
	}

	ret = dma_channel_config_check_parameters(dev, dma_cfg);

	if (ret) {
		return ret;
	}

	ret = dma_channel_config_save_parameters(dev, channel, dma_cfg);
	if (ret) {
		return ret;
	}

	channel_cfg = &data->channels[channel];

	/* To avoid assertions we should first close the driver instance if already enabled
	 */
	if (data->channels[channel].is_configured) {
		config->fsp_api->close(channel_cfg->fsp_ctrl);
	}

	ret = config->fsp_api->open(channel_cfg->fsp_ctrl, &channel_cfg->fsp_cfg);

	if (FSP_SUCCESS != (fsp_err_t)ret) {
		LOG_ERR("%d: Failed to configure %d dma channel with status %d.", __LINE__, channel,
			ret);
		return -EIO;
	}
	/* Mark that requested channel is configured successfully. */
	data->channels[channel].is_configured = true;
	return 0;
}

static int dma_renesas_rz_reload(const struct device *dev, uint32_t channel, uint32_t src,
				 uint32_t dst, size_t size)
{
	const struct dma_renesas_rz_config *config = dev->config;
	struct dma_renesas_rz_data *data = dev->data;

	int ret = dma_channel_common_checks(dev, channel);

	if (ret) {
		return ret;
	}

	if (size == 0) {
		LOG_ERR("%d: Size must to not equal to 0 %d.", __LINE__, size);
		return -EINVAL;
	}

	transfer_info_t *p_info = data->channels[channel].fsp_cfg.p_info;

	p_info->length = size;
	p_info->p_src = (void const *volatile)src;
	p_info->p_dest = (void *volatile)dst;

	ret = config->fsp_api->reconfigure(data->channels[channel].fsp_ctrl, p_info);

	if (FSP_SUCCESS != (fsp_err_t)ret) {
		LOG_ERR("%d: Failed to reload %d dma channel with status %d.", __LINE__, channel,
			ret);
		return -EIO;
	}

	return 0;
}

static int dma_renesas_rz_get_attribute(const struct device *dev, uint32_t type, uint32_t *val)
{
	if (val == NULL) {
		LOG_ERR("%d: Invalid attribute context.", __LINE__);
		return -EINVAL;
	}

	switch (type) {
	case DMA_ATTR_BUFFER_ADDRESS_ALIGNMENT:
	case DMA_ATTR_BUFFER_SIZE_ALIGNMENT:
	case DMA_ATTR_COPY_ALIGNMENT:
		return -ENOSYS;
	case DMA_ATTR_MAX_BLOCK_COUNT:
		/*
		 * this is restricted to 1 because SG and Link Mode configurations are not
		 * supported
		 */
		*val = 1;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static bool dma_renesas_rz_channel_filter(const struct device *dev, int channel, void *filter_param)
{
	ARG_UNUSED(filter_param);

	const struct dma_renesas_rz_config *config = dev->config;
	struct dma_renesas_rz_data *data = dev->data;

	if (channel >= config->num_channels) {
		LOG_ERR("%d: Invalid DMA channel %d.", __LINE__, channel);
		return -EINVAL;
	}

	irq_enable(data->channels[channel].irq);
	/* All DMA channels support triggered by periodic sources so always return true */
	return true;
}

static void dma_renesas_rz_channel_release(const struct device *dev, uint32_t channel)
{
	const struct dma_renesas_rz_config *config = dev->config;
	struct dma_renesas_rz_data *data = dev->data;
	fsp_err_t ret;

	if (channel >= config->num_channels) {
		LOG_ERR("%d: Invalid DMA channel %d.", __LINE__, channel);
	}

	irq_disable(data->channels[channel].irq);
	ret = config->fsp_api->close(data->channels[channel].fsp_ctrl);

	if (ret != FSP_SUCCESS) {
		LOG_ERR("%d: Failed to release %d dma channel with status %d.", __LINE__, channel,
			ret);
	}
}

static DEVICE_API(dma, dma_api) = {
	.reload = dma_renesas_rz_reload,
	.config = dma_renesas_rz_config,
	.start = dma_renesas_rz_start,
	.stop = dma_renesas_rz_stop,
	.suspend = dma_renesas_rz_suspend,
	.resume = dma_renesas_rz_resume,
	.get_status = dma_renesas_rz_get_status,
	.get_attribute = dma_renesas_rz_get_attribute,
	.chan_filter = dma_renesas_rz_channel_filter,
	.chan_release = dma_renesas_rz_channel_release,
};

static int renesas_rz_dma_init(const struct device *dev)
{
	const struct dma_renesas_rz_config *config = dev->config;

	config->irq_configure();

	return 0;
}

static void dmac_err_isr(const void *arg)
{
	ARG_UNUSED(arg);

	/* Call FSP DMAC ERR ISR */
	dmac_b_err_isr();
}

static void dmac_irq_isr(const void *arg)
{
	ARG_UNUSED(arg);

	/* Call FSP DMAC ISR */
	dmac_b_int_isr();
}

#define IRQ_ERR_CONFIGURE(inst, name)                                                              \
	IRQ_CONNECT(                                                                               \
		DT_INST_IRQ_BY_NAME(inst, name, irq), DT_INST_IRQ_BY_NAME(inst, name, priority),   \
		dmac_err_isr, DEVICE_DT_INST_GET(inst),                                            \
		COND_CODE_1(DT_IRQ_HAS_CELL_AT_NAME(DT_DRV_INST(inst), name, flags),           \
				(DT_INST_IRQ_BY_NAME(inst, name, flags)), (0)));  \
	irq_enable(DT_INST_IRQ_BY_NAME(inst, name, irq));

#define IRQ_CONFIGURE(n, inst)                                                                     \
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(inst, n, irq), DT_INST_IRQ_BY_IDX(inst, n, priority),       \
		    dmac_irq_isr, DEVICE_DT_INST_GET(inst),                                        \
		    COND_CODE_1(DT_IRQ_HAS_CELL_AT_IDX(DT_DRV_INST(inst), n, flags),               \
				(DT_INST_IRQ_BY_IDX(inst, n, flags)), (0)));

#define CONFIGURE_ALL_IRQS(inst, n) LISTIFY(n, IRQ_CONFIGURE, (), inst)

#define DMA_RZ_INIT(inst)                                                                          \
	static void dma_rz_##inst##_irq_configure(void)                                            \
	{                                                                                          \
		CONFIGURE_ALL_IRQS(inst, DT_INST_PROP(inst, dma_channels));                        \
		COND_CODE_1(DT_INST_IRQ_HAS_NAME(inst, err1),					\
			    (IRQ_ERR_CONFIGURE(inst, err1)), ())                                \
	}                                                                                          \
                                                                                                   \
	static const struct dma_renesas_rz_config dma_renesas_rz_config_##inst = {                 \
		.unit = inst,                                                                      \
		.num_channels = DT_INST_PROP(inst, dma_channels),                                  \
		.irq_configure = dma_rz_##inst##_irq_configure,                                    \
		.fsp_api = &g_transfer_on_dmac_b};                                                 \
                                                                                                   \
	static dmac_b_instance_ctrl_t g_transfer_ctrl[DT_INST_PROP(inst, dma_channels)];           \
	static transfer_info_t g_transfer_info[DT_INST_PROP(inst, dma_channels)];                  \
	static dmac_b_extended_cfg_t g_transfer_extend[DT_INST_PROP(inst, dma_channels)];          \
	static struct dma_channel_data                                                             \
		dma_rz_##inst##_channels[DT_INST_PROP(inst, dma_channels)] =                       \
			DMA_CHANNEL_ARRAY(inst);                                                   \
                                                                                                   \
	ATOMIC_DEFINE(dma_rz_atomic##inst, DT_INST_PROP(inst, dma_channels));                      \
                                                                                                   \
	static struct dma_renesas_rz_data dma_renesas_rz_data_##inst = {                           \
		.ctx =                                                                             \
			{                                                                          \
				.magic = DMA_MAGIC,                                                \
				.atomic = dma_rz_atomic##inst,                                     \
				.dma_channels = DT_INST_PROP(inst, dma_channels),                  \
			},                                                                         \
		.channels = dma_rz_##inst##_channels};                                             \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, renesas_rz_dma_init, NULL, &dma_renesas_rz_data_##inst,        \
			      &dma_renesas_rz_config_##inst, PRE_KERNEL_1,                         \
			      CONFIG_DMA_INIT_PRIORITY, &dma_api);

DT_INST_FOREACH_STATUS_OKAY(DMA_RZ_INIT);
