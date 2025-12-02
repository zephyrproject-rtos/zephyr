/*
 * Copyright (c) 2024-2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT renesas_rz_dmac_b

#include <zephyr/drivers/dma.h>
#include <zephyr/logging/log.h>
#include "dma_renesas_rz.h"

#ifdef CONFIG_CPU_CORTEX_A
#include <zephyr/cache.h>
#endif /* CONFIG_CPU_CORTEX_A */

#ifdef CONFIG_USE_RZ_FSP_DMAC_B
#include "r_dmac_b.h"

static const transfer_api_t *const rz_g_transfer_on_dma = &g_transfer_on_dmac_b;
#define dma_instance_ctrl_t dmac_b_instance_ctrl_t
#define dma_extended_cfg_t  dmac_b_extended_cfg_t
#define dma_callback_args_t dmac_b_callback_args_t
#define dma_extended_info_t dmac_b_extended_info_t

void dmac_b_int_isr(void *irq);
void dmac_b_err_isr(void *irq);
#define RZ_DMA_INT_ISR(irq) dmac_b_int_isr((void *)irq)
#define RZ_DMA_ERR_ISR(irq) dmac_b_err_isr((void *)irq)

#else /* CONFIG_USE_RZ_FSP_DMAC */
#include "r_dmac.h"

static const transfer_api_t *const rz_g_transfer_on_dma = &g_transfer_on_dmac;
#define dma_instance_ctrl_t dmac_instance_ctrl_t
#define dma_extended_cfg_t  dmac_extended_cfg_t

#ifdef CONFIG_CPU_CORTEX_A
#define dma_callback_args_t dmac_callback_args_t
#define dma_extended_info_t dmac_extended_info_t
void dmac_err_isr(void *irq);
#define RZ_DMA_ERR_ISR(irq) dmac_err_isr((void *)irq)
#else /* CONFIG_CPU_AARCH32_CORTEX_R */
#define dma_callback_args_t transfer_callback_args_t

#define RZ_MASTER_MPU_STADD_DISABLE_RW_PROTECTION  (0x00000000)
#define RZ_MASTER_MPU_ENDADD_DISABLE_RW_PROTECTION (0x00000C00)
#endif /* CONFIG_CPU_CORTEX_A */

void dmac_int_isr(void *irq);
#define RZ_DMA_INT_ISR(irq) dmac_int_isr((void *)irq)

#endif /* CONFIG_USE_RZ_FSP_DMAC_B */

#define RZ_DMA_CHANNEL_SCHEDULING_FIXED       0
#define RZ_DMA_CHANNEL_SCHEDULING_ROUND_ROBIN 1
#define RZ_DMA_MODE_SELECT_REGISTER           0
#define RZ_DMA_MODE_SELECT_LINK               1
#define RZ_DMA_ACK_MODE_MASK_DACK_OUTPUT      4

#define RZ_DMA_REQUEST_DIRECTION_SOURCE_MODULE      0
#define RZ_DMA_REQUEST_DIRECTION_DESTINATION_MODULE 1

#define RZ_DMA_GRP_CH_CHCTRL_SETSUS_Msk (0x100UL)
#define RZ_DMA_GRP_CH_CHCTRL_CLRSUS_Msk (0x200UL)

LOG_MODULE_REGISTER(renesas_rz_dma);

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
#if defined(CONFIG_CPU_CORTEX_M) || defined(CONFIG_CPU_CORTEX_A)
#ifdef CONFIG_DMA_64BIT
	uint64_t err_irq;
#else  /* !CONFIG_DMA_64BIT */
	uint32_t err_irq;
#endif /* CONFIG_DMA_64BIT */
#endif /* defined(CONFIG_CPU_CORTEX_M) || defined(CONFIG_CPU_CORTEX_A) */
};

static void dma_rz_cb_handler(dma_callback_args_t *args)
{
	struct dmac_cb_ctx *cb_ctx = (struct dmac_cb_ctx *)args->p_context;
	uint32_t channel = cb_ctx->channel;
	const struct device *dev = cb_ctx->dmac_dev;
	struct dma_renesas_rz_data *data = dev->data;
	dma_callback_t user_cb = data->channels[channel].user_cb;
	void *user_data = data->channels[channel].user_data;

	if (user_cb) {
		user_cb(dev, user_data, channel, DMA_STATUS_COMPLETE);
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
		LOG_ERR("%d:Channel chaining is not supported.", __LINE__);
		return -ENOTSUP;
	}

	if (cfg->head_block->dest_scatter_count || cfg->head_block->source_gather_count ||
	    cfg->head_block->source_gather_interval || cfg->head_block->dest_scatter_interval) {
		LOG_ERR("%d: Scatter and gather are not supported.", __LINE__);
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
	default:
		LOG_ERR("%d: Unsupported data width.", __LINE__);
		return -ENOTSUP;
	}

	return transfer_size;
}

static inline int dma_channel_config_save_parameters(const struct device *dev, uint32_t channel,
						     struct dma_config *cfg)
{
	struct dma_renesas_rz_data *data = dev->data;
	transfer_info_t *p_info = data->channels[channel].fsp_cfg.p_info;
	dma_extended_cfg_t *p_extend =
		(dma_extended_cfg_t *)data->channels[channel].fsp_cfg.p_extend;
	transfer_addr_mode_t src_transfer_addr_mode;
	transfer_addr_mode_t dest_transfer_addr_mode;

	transfer_mode_t transfer_mode;
	bool activation_with_software_trigger;

	/* Save transfer properties required by FSP */
	switch (cfg->head_block->dest_addr_adj) {
	case DMA_ADDR_ADJ_NO_CHANGE:
		dest_transfer_addr_mode = TRANSFER_ADDR_MODE_FIXED;
		break;
	case DMA_ADDR_ADJ_INCREMENT:
		dest_transfer_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED;
		break;
	default:
		LOG_ERR("%d, Unsupported destination address adjustment.", __LINE__);
		return -ENOTSUP;
	}

	switch (cfg->head_block->source_addr_adj) {
	case DMA_ADDR_ADJ_NO_CHANGE:
		src_transfer_addr_mode = TRANSFER_ADDR_MODE_FIXED;
		break;
	case DMA_ADDR_ADJ_INCREMENT:
		src_transfer_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED;
		break;
	default:
		LOG_ERR("%d, Unsupported source address adjustment.", __LINE__);
		return -ENOTSUP;
	}

	switch (cfg->channel_direction) {
	case MEMORY_TO_MEMORY:
		transfer_mode = TRANSFER_MODE_BLOCK;
		p_extend->activation_request_source_select =
			RZ_DMA_REQUEST_DIRECTION_DESTINATION_MODULE;
		activation_with_software_trigger = true;
		break;
	case PERIPHERAL_TO_MEMORY:
		transfer_mode = TRANSFER_MODE_NORMAL;
		p_extend->activation_request_source_select =
			RZ_DMA_REQUEST_DIRECTION_DESTINATION_MODULE;
		activation_with_software_trigger = false;
		break;
	case MEMORY_TO_PERIPHERAL:
		transfer_mode = TRANSFER_MODE_NORMAL;
		p_extend->activation_request_source_select = RZ_DMA_REQUEST_DIRECTION_SOURCE_MODULE;
		activation_with_software_trigger = false;
		break;
	default:
		LOG_ERR("%d: Unsupported direction mode.", __LINE__);
		return -ENOTSUP;
	}

#ifdef CONFIG_CPU_CORTEX_A
	dma_extended_info_t *p_extend_info = (dma_extended_info_t *)p_info->p_extend_info;

	p_extend->continuous_setting = DMAC_CONTINUOUS_SETTING_TRANSFER_NEXT0_ONCE;

	p_extend->detection_mode = DMAC_DETECTION_RISING_EDGE;

	if (cfg->head_block->block_size > UINT16_MAX) {
		LOG_ERR("Larger than max block size");
		return -ENOTSUP;
	}

	/* Convert data size following FSP convention */
	p_extend_info->src_size = dma_channel_set_size(cfg->source_data_size);
	p_extend_info->dest_size = dma_channel_set_size(cfg->dest_data_size);

	p_info->transfer_settings_word_b.dest_addr_mode = dest_transfer_addr_mode;
	p_info->transfer_settings_word_b.src_addr_mode = src_transfer_addr_mode;
	p_info->transfer_settings_word_b.mode = transfer_mode;

	p_extend->activation_source = activation_with_software_trigger
					      ? DMAC_TRIGGER_EVENT_SOFTWARE_TRIGGER
					      : cfg->dma_slot;

#else /* CONFIG_CPU_CORTEX_M || CONFIG_CPU_AARCH32_CORTEX_R */
	const struct dma_renesas_rz_config *config = dev->config;

	p_extend->unit = config->unit;

	/* Convert data size following FSP convention */
	p_info->src_size = dma_channel_set_size(cfg->source_data_size);
	p_info->dest_size = dma_channel_set_size(cfg->dest_data_size);

	p_info->p_next1_src = NULL;
	p_info->p_next1_dest = NULL;
	p_info->next1_length = 1;

	p_info->dest_addr_mode = dest_transfer_addr_mode;
	p_info->src_addr_mode = src_transfer_addr_mode;
	p_info->mode = transfer_mode;

#ifdef CONFIG_CPU_CORTEX_M
	p_extend->continuous_setting = DMAC_B_CONTINUOUS_SETTING_TRANSFER_ONCE;

	p_extend->external_detection_mode = DMAC_B_EXTERNAL_DETECTION_NO_DETECTION;
	p_extend->internal_detection_mode = DMAC_B_INTERNAL_DETECTION_NO_DETECTION;

	p_extend->activation_source = activation_with_software_trigger
					      ? DMAC_TRIGGER_EVENT_SOFTWARE_TRIGGER
					      : cfg->dma_slot;

#else /* CONFIG_CPU_AARCH32_CORTEX_R */
	p_extend->activation_source =
		activation_with_software_trigger ? ELC_EVENT_NONE : cfg->dma_slot;
#endif

#endif

	p_extend->ack_mode = RZ_DMA_ACK_MODE_MASK_DACK_OUTPUT;
	/* Save transfer properties required by FSP */
	p_info->p_src = (void const *volatile)cfg->head_block->source_address;
	p_info->p_dest = (void *volatile)cfg->head_block->dest_address;
	p_info->length = cfg->head_block->block_size;

	p_extend->channel = channel;

	/* Save INTID and priority */
	p_extend->dmac_int_irq = data->channels[channel].irq;
	p_extend->dmac_int_ipl = data->channels[channel].irq_ipl;

	/* Save callback and data and use the generic handler. */
	data->channels[channel].user_cb = cfg->dma_callback;
	data->channels[channel].user_data = cfg->user_data;
	data->channels[channel].cb_ctx.dmac_dev = dev;
	data->channels[channel].cb_ctx.channel = channel;
	p_extend->p_callback = dma_rz_cb_handler;
	p_extend->p_context = (void *)&data->channels[channel].cb_ctx;

	data->channels[channel].direction = cfg->channel_direction;

	/*
	 * Only support two priority modes, 0 is the highest priority with respect to FIXED
	 * Priority, Round Robin otherwise.
	 */
	if (cfg->channel_priority == 0) {
		p_extend->channel_scheduling = RZ_DMA_CHANNEL_SCHEDULING_FIXED;
	} else {
		p_extend->channel_scheduling = RZ_DMA_CHANNEL_SCHEDULING_ROUND_ROBIN;
	}

	if (1 < cfg->block_count) {
		LOG_ERR("%d: Link Mode is not supported.", __LINE__);
	} else {
		p_extend->dmac_mode = RZ_DMA_MODE_SELECT_REGISTER;
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

	dma_instance_ctrl_t *p_ctrl = (dma_instance_ctrl_t *)data->channels[channel].fsp_ctrl;

#ifdef CONFIG_CPU_CORTEX_A
	/* Set transfer status is suspend */
	p_ctrl->p_reg->CHCTRL = RZ_DMA_GRP_CH_CHCTRL_SETSUS_Msk;

	/* Check whether a transfer is suspended. */
	FSP_HARDWARE_REGISTER_WAIT(p_ctrl->p_reg->CHSTAT_b.SUS, 1);
#else /* CONFIG_CPU_CORTEX_M || CONFIG_CPU_AARCH32_CORTEX_R */
	uint8_t group = RZ_DMA_PRV_GROUP(channel);
	uint8_t prv_channel = RZ_DMA_PRV_CHANNEL(channel);

	/* Set transfer status is suspend */
	p_ctrl->p_reg->GRP[group].CH[prv_channel].CHCTRL = RZ_DMA_GRP_CH_CHCTRL_SETSUS_Msk;

	/* Check whether a transfer is suspended. */
	FSP_HARDWARE_REGISTER_WAIT(p_ctrl->p_reg->GRP[group].CH[prv_channel].CHSTAT_b.SUS, 1);
#endif

	return 0;
}

static int dma_renesas_rz_resume(const struct device *dev, uint32_t channel)
{
	struct dma_renesas_rz_data *data = dev->data;

	int ret = dma_channel_common_checks(dev, channel);

	if (ret) {
		return ret;
	}

	dma_instance_ctrl_t *p_ctrl = (dma_instance_ctrl_t *)data->channels[channel].fsp_ctrl;

#ifdef CONFIG_CPU_CORTEX_A
	/* Check whether a transfer is suspended. */
	if (0 == p_ctrl->p_reg->CHSTAT_b.SUS) {
		LOG_ERR("%d: DMA channel not suspend.", channel);
		return -EINVAL;
	}

	/* Restore transfer status from suspend */
	p_ctrl->p_reg->CHCTRL |= RZ_DMA_GRP_CH_CHCTRL_CLRSUS_Msk;
#else /* CONFIG_CPU_CORTEX_M || CONFIG_CPU_AARCH32_CORTEX_R */
	uint8_t group = RZ_DMA_PRV_GROUP(channel);
	uint8_t prv_channel = RZ_DMA_PRV_CHANNEL(channel);

	/* Check whether a transfer is suspended. */
	if (0 == p_ctrl->p_reg->GRP[group].CH[prv_channel].CHSTAT_b.SUS) {
		LOG_ERR("%d: DMA channel not suspend.", channel);
		return -EINVAL;
	}

	/* Restore transfer status from suspend */
	p_ctrl->p_reg->GRP[group].CH[prv_channel].CHCTRL |= RZ_DMA_GRP_CH_CHCTRL_CLRSUS_Msk;
#endif

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
	dma_extended_cfg_t const *p_extend;
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

#ifdef CONFIG_CPU_CORTEX_A
	/* Ensure cache coherency before starting DMA */
	transfer_info_t *p_info = data->channels[channel].fsp_cfg.p_info;

	sys_cache_data_flush_range((void *)p_info->p_src, p_info->length);
	sys_cache_data_flush_range((void *)p_info->p_dest, p_info->length);
#endif /* CONFIG_CPU_CORTEX_A */

#if defined(CONFIG_CPU_CORTEX_M) || defined(CONFIG_CPU_CORTEX_A)
	if (DMAC_TRIGGER_EVENT_SOFTWARE_TRIGGER == p_extend->activation_source) {
#else  /* CONFIG_CPU_AARCH32_CORTEX_R */
	if (ELC_EVENT_NONE == p_extend->activation_source) {
#endif /* defined(CONFIG_CPU_CORTEX_M) || defined(CONFIG_CPU_CORTEX_A) */
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

static int dma_renesas_rz_configure(const struct device *dev, uint32_t channel,
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

	/* To avoid assertions we should first close the driver instance if already enabled */
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

#ifdef CONFIG_DMA_64BIT
static int dma_renesas_rz_reload(const struct device *dev, uint32_t channel, uint64_t src,
				 uint64_t dst, size_t size)
#else  /* !CONFIG_DMA_64BIT */
static int dma_renesas_rz_reload(const struct device *dev, uint32_t channel, uint32_t src,
				 uint32_t dst, size_t size)
#endif /* CONFIG_DMA_64BIT */
{
	const struct dma_renesas_rz_config *config = dev->config;
	struct dma_renesas_rz_data *data = dev->data;

	int ret = dma_channel_common_checks(dev, channel);

	if (ret) {
		return ret;
	}

	if (size == 0) {
		LOG_ERR("%d: Size must to not equal to 0.", __LINE__);
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
		 * This is restricted to 1 because SG and Link Mode configurations are not
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
		return false;
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
		return;
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
	.config = dma_renesas_rz_configure,
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

#ifdef CONFIG_CPU_AARCH32_CORTEX_R
	uint8_t region_num = BSP_FEATURE_BSP_MASTER_MPU_REGION_TYPE == 1 ? 8 : 16;

	/* Disable register protection for Master-MPU related registers. */
	R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_SYSTEM);

	if (config->unit == 0) {
		for (uint8_t i = 0; i < region_num; i++) {
			R_MPU0->RGN[i].STADD = RZ_MASTER_MPU_STADD_DISABLE_RW_PROTECTION;
			R_MPU0->RGN[i].ENDADD = RZ_MASTER_MPU_ENDADD_DISABLE_RW_PROTECTION;
		}
	}
	if (config->unit == 1) {
		for (uint8_t i = 0; i < region_num; i++) {
			R_MPU1->RGN[i].STADD = RZ_MASTER_MPU_STADD_DISABLE_RW_PROTECTION;
			R_MPU1->RGN[i].ENDADD = RZ_MASTER_MPU_ENDADD_DISABLE_RW_PROTECTION;
		}
	}

	/* Enable register protection for Master-MPU related registers. */
	R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_SYSTEM);
#endif /* CONFIG_CPU_AARCH32_CORTEX_R */

	return 0;
}

static void rz_dma_int_isr(void *arg)
{
	struct dma_channel_data *channel_data = (struct dma_channel_data *)arg;

	dma_extended_cfg_t *p_extend = (dma_extended_cfg_t *)channel_data->fsp_cfg.p_extend;

#ifdef CONFIG_CPU_CORTEX_A
	transfer_info_t *p_info = channel_data->fsp_cfg.p_info;

	sys_cache_data_invd_range((void *)p_info->p_dest, p_info->length);
#endif /* CONFIG_CPU_CORTEX_A */

	RZ_DMA_INT_ISR(p_extend->dmac_int_irq);
}

#if defined(CONFIG_CPU_CORTEX_M) || defined(CONFIG_CPU_CORTEX_A)
static void rz_dma_err_isr(const struct device *dev)
{
	struct dma_renesas_rz_data *data = dev->data;

	RZ_DMA_ERR_ISR(data->err_irq);
}

#define RZ_DMA_DATA_STRUCT_GET_ERR_IRQ(inst, err_name)                                             \
	.err_irq = DT_INST_IRQ_BY_NAME(inst, err_name, irq),
#else /* CONFIG_CPU_AARCH32_CORTEX_R */
#define RZ_DMA_DATA_STRUCT_GET_ERR_IRQ(inst, err_name)
#endif /* defined(CONFIG_CPU_CORTEX_M) || defined(CONFIG_CPU_CORTEX_A) */

#define RZ_DMA_IRQ_ERR_CONFIGURE(inst, name)                                                       \
	IRQ_CONNECT(                                                                               \
		DT_INST_IRQ_BY_NAME(inst, name, irq), DT_INST_IRQ_BY_NAME(inst, name, priority),   \
		rz_dma_err_isr, DEVICE_DT_INST_GET(inst),                                          \
		COND_CODE_1(DT_IRQ_HAS_CELL_AT_NAME(DT_DRV_INST(inst), name, flags),               \
				(DT_INST_IRQ_BY_NAME(inst, name, flags)), (0)));  \
	irq_enable(DT_INST_IRQ_BY_NAME(inst, name, irq));

#define RZ_DMA_IRQ_CONFIGURE(n, inst)                                                              \
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(inst, n, irq), DT_INST_IRQ_BY_IDX(inst, n, priority),       \
		    rz_dma_int_isr, &dma_rz_##inst##_channels[n],                                  \
		    COND_CODE_1(DT_IRQ_HAS_CELL_AT_IDX(DT_DRV_INST(inst), n, flags),               \
				(DT_INST_IRQ_BY_IDX(inst, n, flags)), (0)));

#define RZ_DMA_CONFIGURE_ALL_IRQS(inst, n) LISTIFY(n, RZ_DMA_IRQ_CONFIGURE, (), inst)

#define RZ_DMA_GET_UNIT(inst)                                                                      \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, dma_unit),                                         \
				(DT_INST_PROP(inst, dma_unit)), (0))

#define DMA_RZ_INIT(inst)                                                                          \
	static dma_instance_ctrl_t g_transfer_ctrl[DT_INST_PROP(inst, dma_channels)];              \
	RZ_DMA_EXTERN_INFO_DECLARATION(DT_INST_PROP(inst, dma_channels));                          \
	static transfer_info_t g_transfer_info[DT_INST_PROP(inst, dma_channels)] =                 \
		RZ_DMA_TRANSFER_INFO_ARRAY(inst);                                                  \
	static dma_extended_cfg_t g_transfer_extend[DT_INST_PROP(inst, dma_channels)];             \
	static struct dma_channel_data                                                             \
		dma_rz_##inst##_channels[DT_INST_PROP(inst, dma_channels)] =                       \
			RZ_DMA_CHANNEL_DATA_ARRAY(inst);                                           \
                                                                                                   \
	ATOMIC_DEFINE(dma_rz_atomic##inst, DT_INST_PROP(inst, dma_channels));                      \
	static void dma_rz_##inst##_irq_configure(void)                                            \
	{                                                                                          \
		RZ_DMA_CONFIGURE_ALL_IRQS(inst, DT_INST_PROP(inst, dma_channels));                 \
		COND_CODE_1(DT_INST_IRQ_HAS_NAME(inst, err1),                         \
				(RZ_DMA_IRQ_ERR_CONFIGURE(inst, err1)), ())            \
	}                                                                                          \
                                                                                                   \
	static const struct dma_renesas_rz_config dma_renesas_rz_config_##inst = {                 \
		.unit = RZ_DMA_GET_UNIT(inst),                                                     \
		.num_channels = DT_INST_PROP(inst, dma_channels),                                  \
		.irq_configure = dma_rz_##inst##_irq_configure,                                    \
		.fsp_api = rz_g_transfer_on_dma};                                                  \
                                                                                                   \
	static struct dma_renesas_rz_data dma_renesas_rz_data_##inst = {                           \
		.ctx =                                                                             \
			{                                                                          \
				.magic = DMA_MAGIC,                                                \
				.atomic = dma_rz_atomic##inst,                                     \
				.dma_channels = DT_INST_PROP(inst, dma_channels),                  \
			},                                                                         \
		.channels = dma_rz_##inst##_channels,                                              \
		RZ_DMA_DATA_STRUCT_GET_ERR_IRQ(inst, err1)};                                       \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, renesas_rz_dma_init, NULL, &dma_renesas_rz_data_##inst,        \
			      &dma_renesas_rz_config_##inst, PRE_KERNEL_1,                         \
			      CONFIG_DMA_INIT_PRIORITY, &dma_api);

DT_INST_FOREACH_STATUS_OKAY(DMA_RZ_INIT);

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT renesas_rz_dmac

DT_INST_FOREACH_STATUS_OKAY(DMA_RZ_INIT);
