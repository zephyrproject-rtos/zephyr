/*
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <Adc_Sar_Ip_HwAccess.h>
#include <Adc_Sar_Ip.h>
#include <Adc_Sar_Ip_Irq.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#define DT_DRV_COMPAT nxp_s32_adc_sar
LOG_MODULE_REGISTER(adc_nxp_s32_adc_sar, CONFIG_ADC_LOG_LEVEL);

/* Convert channel of group ADC to channel of physical ADC instance */
#define ADC_NXP_S32_GROUPCHAN_2_PHYCHAN(group, channel)	\
						(ADC_SAR_IP_HW_REG_SIZE * group + channel)

struct adc_nxp_s32_config {
	ADC_Type *base;
	uint8_t instance;
	uint8_t group_channel;
	uint8_t callback_select;
	Adc_Sar_Ip_ConfigType *adc_cfg;
	void (*irq_config_func)(const struct device *dev);
	const struct pinctrl_dev_config *pin_cfg;
};

struct adc_nxp_s32_data {
	const struct device *dev;
	struct adc_context ctx;
	uint16_t *buffer;
	uint16_t *buf_end;
	uint16_t *repeat_buffer;
	uint32_t mask_channels;
	uint8_t num_channels;
};

static int adc_nxp_s32_init(const struct device *dev)
{
	const struct adc_nxp_s32_config *config = dev->config;
	struct adc_nxp_s32_data *data = dev->data;
	Adc_Sar_Ip_StatusType status;
	/* This array shows max number of channels of each group */
	uint8_t map_chan_group[ADC_SAR_IP_INSTANCE_COUNT][ADC_SAR_IP_NUM_GROUP_CHAN]
							= FEATURE_ADC_MAX_CHN_COUNT;

	data->num_channels = map_chan_group[config->instance][config->group_channel];

	if (config->pin_cfg) {
		if (pinctrl_apply_state(config->pin_cfg, PINCTRL_STATE_DEFAULT)) {
			return -EIO;
		}
	}

	status = Adc_Sar_Ip_Init(config->instance, config->adc_cfg);
	if (status) {
		return -EIO;
	}

#if FEATURE_ADC_HAS_CALIBRATION
	status = Adc_Sar_Ip_DoCalibration(config->instance);
	if (status) {
		return -EIO;
	}
#endif

	Adc_Sar_Ip_EnableNotifications(config->instance,
				config->callback_select ?
					ADC_SAR_IP_NOTIF_FLAG_NORMAL_ENDCHAIN
					: ADC_SAR_IP_NOTIF_FLAG_NORMAL_EOC);

	data->dev = dev;
	config->irq_config_func(dev);

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static int adc_nxp_s32_channel_setup(const struct device *dev,
				const struct adc_channel_cfg *channel_cfg)
{
	struct adc_nxp_s32_data *data = dev->data;

	if (channel_cfg->channel_id >= data->num_channels) {
		LOG_ERR("Channel %d is not valid", channel_cfg->channel_id);
		return -EINVAL;
	}

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("Unsupported channel acquisition time");
		return -ENOTSUP;
	}

	if (channel_cfg->differential) {
		LOG_ERR("Differential channels are not supported");
		return -ENOTSUP;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("Unsupported channel gain %d", channel_cfg->gain);
		return -ENOTSUP;
	}

	if (channel_cfg->reference != ADC_REF_INTERNAL) {
		LOG_ERR("Unsupported channel reference");
		return -ENOTSUP;
	}

	return 0;
}

static int adc_nxp_s32_validate_buffer_size(const struct device *dev,
					const struct adc_sequence *sequence)
{
	uint8_t active_channels = 0;
	size_t needed_size;

	active_channels = POPCOUNT(sequence->channels);

	needed_size = active_channels * sizeof(uint16_t);
	if (sequence->options) {
		needed_size *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed_size) {
		return -ENOSPC;
	}

	return 0;
}

#if FEATURE_ADC_HAS_AVERAGING
static int adc_nxp_s32_set_averaging(const struct device *dev, uint8_t oversampling)
{
	const struct adc_nxp_s32_config *config = dev->config;
	Adc_Sar_Ip_AvgSelectType avg_sel = ADC_SAR_IP_AVG_4_CONV;
	bool avg_en = true;

	switch (oversampling) {
	case 0:
		avg_en = false;
		break;
	case 2:
		avg_sel = ADC_SAR_IP_AVG_4_CONV;
		break;
	case 3:
		avg_sel = ADC_SAR_IP_AVG_8_CONV;
		break;
	case 4:
		avg_sel = ADC_SAR_IP_AVG_16_CONV;
		break;
	case 5:
		avg_sel = ADC_SAR_IP_AVG_32_CONV;
		break;
	default:
		LOG_ERR("Unsupported oversampling value");
		return -ENOTSUP;
	}
	Adc_Sar_Ip_SetAveraging(config->instance, avg_en, avg_sel);

	return 0;
}
#endif

#if (ADC_SAR_IP_SET_RESOLUTION == STD_ON)
static int adc_nxp_s32_set_resolution(const struct device *dev, uint8_t adc_resol)
{
	const struct adc_nxp_s32_config *config = dev->config;
	Adc_Sar_Ip_Resolution resolution;

	switch (adc_resol) {
	case 8:
		resolution = ADC_SAR_IP_RESOLUTION_8;
		break;
	case 10:
		resolution = ADC_SAR_IP_RESOLUTION_10;
		break;
	case 12:
		resolution = ADC_SAR_IP_RESOLUTION_12;
		break;
	case 14:
		resolution = ADC_SAR_IP_RESOLUTION_14;
		break;
	default:
		LOG_ERR("Unsupported resolution");
		return -ENOTSUP;
	}
	Adc_Sar_Ip_SetResolution(config->instance, resolution);

	return 0;
}
#endif

static int adc_nxp_s32_start_read_async(const struct device *dev,
				 const struct adc_sequence *sequence)
{
	const struct adc_nxp_s32_config *config = dev->config;
	struct adc_nxp_s32_data *data = dev->data;
	int error;
	uint32_t mask;
	uint8_t channel;

	if (find_msb_set(sequence->channels) > data->num_channels) {
		LOG_ERR("Channels out of bit map");
		return -EINVAL;
	}

	error = adc_nxp_s32_validate_buffer_size(dev, sequence);
	if (error) {
		LOG_ERR("Buffer size isn't enough");
		return -EINVAL;
	}

#if FEATURE_ADC_HAS_AVERAGING
	error = adc_nxp_s32_set_averaging(dev, sequence->oversampling);
	if (error) {
		return -ENOTSUP;
	}
#else
	if (sequence->oversampling) {
		LOG_ERR("Oversampling can't be changed");
		return -ENOTSUP;
	}
#endif

#if (ADC_SAR_IP_SET_RESOLUTION == STD_ON)
	error = adc_nxp_s32_set_resolution(dev, sequence->resolution);
	if (error) {
		return -ENOTSUP;
	}
#else
	if (sequence->resolution != ADC_SAR_IP_MAX_RESOLUTION) {
		LOG_ERR("Resolution can't be changed");
		return -ENOTSUP;
	}
#endif

	if (sequence->calibrate) {
#if FEATURE_ADC_HAS_CALIBRATION
		error = Adc_Sar_Ip_DoCalibration(config->instance);
		if (error) {
			LOG_ERR("Error during calibration");
			return -EIO;
		}
#else
		LOG_ERR("Unsupported calibration");
		return -ENOTSUP;
#endif
	}

	for (int i = 0; i < data->num_channels; i++) {
		mask = (sequence->channels >> i) & 0x1;
		channel = ADC_NXP_S32_GROUPCHAN_2_PHYCHAN(config->group_channel, i);
		if (mask) {
			Adc_Sar_Ip_EnableChannelNotifications(config->instance,
						channel, ADC_SAR_IP_CHAN_NOTIF_EOC);
			Adc_Sar_Ip_EnableChannel(config->instance,
						ADC_SAR_IP_CONV_CHAIN_NORMAL, channel);
		} else {
			Adc_Sar_Ip_DisableChannelNotifications(config->instance,
						channel, ADC_SAR_IP_CHAN_NOTIF_EOC);
			Adc_Sar_Ip_DisableChannel(config->instance,
						ADC_SAR_IP_CONV_CHAIN_NORMAL, channel);
		}
	}

	/* Save ADC sequence sampling buffer and its end pointer address */
	data->buffer = sequence->buffer;
	if (config->callback_select) {
		data->buf_end = data->buffer + sequence->buffer_size / sizeof(uint16_t);
	}

	adc_context_start_read(&data->ctx, sequence);
	error = adc_context_wait_for_completion(&data->ctx);

	return error;
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_nxp_s32_data *data = CONTAINER_OF(ctx, struct adc_nxp_s32_data, ctx);
	const struct adc_nxp_s32_config *config = data->dev->config;

	data->mask_channels = ctx->sequence.channels;
	data->repeat_buffer = data->buffer;

	Adc_Sar_Ip_StartConversion(config->instance, ADC_SAR_IP_CONV_CHAIN_NORMAL);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx,
						bool repeat_sampling)
{
	struct adc_nxp_s32_data *const data =
		CONTAINER_OF(ctx, struct adc_nxp_s32_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static int adc_nxp_s32_read_async(const struct device *dev,
				 const struct adc_sequence *sequence,
				 struct k_poll_signal *async)
{
	struct adc_nxp_s32_data *data = dev->data;
	int error = 0;

	adc_context_lock(&data->ctx, async ? true : false, async);
	error = adc_nxp_s32_start_read_async(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}

static int adc_nxp_s32_read(const struct device *dev,
			  const struct adc_sequence *sequence)
{
	return adc_nxp_s32_read_async(dev, sequence, NULL);
}

static void adc_nxp_s32_isr(const struct device *dev)
{
	const struct adc_nxp_s32_config *config = dev->config;

	Adc_Sar_Ip_IRQHandler(config->instance);
}

#define ADC_NXP_S32_DRIVER_API(n)						\
	static DEVICE_API(adc, adc_nxp_s32_driver_api_##n) = {			\
		.channel_setup = adc_nxp_s32_channel_setup,			\
		.read = adc_nxp_s32_read,					\
		IF_ENABLED(CONFIG_ADC_ASYNC, (.read_async = adc_nxp_s32_read_async,))\
		.ref_internal = DT_INST_PROP(n, vref_mv),			\
	};

#define ADC_NXP_S32_IRQ_CONFIG(n)						\
	static void adc_nxp_s32_adc_sar_config_func_##n(const struct device *dev)\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n),					\
			DT_INST_IRQ(n, priority),				\
			adc_nxp_s32_isr, DEVICE_DT_INST_GET(n), 0);		\
		irq_enable(DT_INST_IRQN(n));					\
	};

#define ADC_NXP_S32_CALLBACK_DEFINE(n)						\
	void adc_nxp_s32_normal_end_conversion_callback##n(const uint16 PhysicalChanId)\
	{									\
		const struct device *dev = DEVICE_DT_INST_GET(n);		\
		const struct adc_nxp_s32_config *config = dev->config;		\
		struct adc_nxp_s32_data *data = dev->data;			\
		uint16_t result = 0;						\
										\
		result = Adc_Sar_Ip_GetConvData(n, PhysicalChanId);		\
		LOG_DBG("End conversion, channel %d, group %d, result = %d",	\
					ADC_SAR_IP_CHAN_2_BIT(PhysicalChanId),	\
					config->group_channel, result);		\
										\
		*data->buffer++ = result;					\
		data->mask_channels &=						\
				~BIT(ADC_SAR_IP_CHAN_2_BIT(PhysicalChanId));	\
										\
		if (!data->mask_channels) {					\
			adc_context_on_sampling_done(&data->ctx,		\
							(struct device *)dev);	\
		}								\
	};									\
	void adc_nxp_s32_normal_endchain_callback##n(void)			\
	{									\
		const struct device *dev = DEVICE_DT_INST_GET(n);		\
		const struct adc_nxp_s32_config *config = dev->config;		\
		struct adc_nxp_s32_data *data = dev->data;			\
		uint16_t result = 0;						\
		uint8_t channel;						\
										\
		while (data->mask_channels) {					\
			channel = ADC_NXP_S32_GROUPCHAN_2_PHYCHAN(		\
					config->group_channel,			\
					(find_lsb_set(data->mask_channels)-1));	\
			result = Adc_Sar_Ip_GetConvData(n, channel);		\
			LOG_DBG("End chain, channel %d, group %d, result = %d",	\
					ADC_SAR_IP_CHAN_2_BIT(channel),		\
					config->group_channel, result);		\
			if (data->buffer < data->buf_end) {			\
				*data->buffer++ = result;			\
			}							\
			data->mask_channels &=					\
					~BIT(ADC_SAR_IP_CHAN_2_BIT(channel));	\
		}								\
										\
		adc_context_on_sampling_done(&data->ctx, (struct device *)dev);	\
	};

#define ADC_NXP_S32_INSTANCE_CHECK(indx, n)	\
	((DT_INST_REG_ADDR(n) == IP_ADC_##indx##_BASE) ? indx : 0)
#define ADC_NXP_S32_GET_INSTANCE(n)		\
	LISTIFY(__DEBRACKET ADC_INSTANCE_COUNT, ADC_NXP_S32_INSTANCE_CHECK, (|), n)

#if (FEATURE_ADC_HAS_HIGH_SPEED_ENABLE == 1U)
#define ADC_NXP_S32_HIGH_SPEED_CFG(n) .HighSpeedConvEn = DT_INST_PROP(n, high_speed),
#else
#define ADC_NXP_S32_HIGH_SPEED_CFG(n)
#endif

#if (ADC_SAR_IP_SET_RESOLUTION == STD_ON)
#define ADC_NXP_S32_RESOLUTION_CFG(n) .AdcResolution = ADC_SAR_IP_RESOLUTION_14,
#else
#define ADC_NXP_S32_RESOLUTION_CFG(n)
#endif

#define ADC_NXP_S32_INIT_DEVICE(n)						\
	ADC_NXP_S32_DRIVER_API(n)						\
	ADC_NXP_S32_CALLBACK_DEFINE(n)						\
	ADC_NXP_S32_IRQ_CONFIG(n)						\
	COND_CODE_1(DT_INST_NUM_PINCTRL_STATES(n),				\
				(PINCTRL_DT_INST_DEFINE(n);), (EMPTY))		\
	static const Adc_Sar_Ip_ConfigType adc_nxp_s32_default_config##n =	\
	{									\
		.ConvMode = ADC_SAR_IP_CONV_MODE_ONESHOT,			\
		ADC_NXP_S32_RESOLUTION_CFG(n)					\
		ADC_NXP_S32_HIGH_SPEED_CFG(n)					\
		.EndOfNormalChainNotification =					\
				adc_nxp_s32_normal_endchain_callback##n,	\
		.EndOfConvNotification =					\
				adc_nxp_s32_normal_end_conversion_callback##n,	\
	};									\
	static struct adc_nxp_s32_data adc_nxp_s32_data_##n = {			\
		ADC_CONTEXT_INIT_TIMER(adc_nxp_s32_data_##n, ctx),		\
		ADC_CONTEXT_INIT_LOCK(adc_nxp_s32_data_##n, ctx),		\
		ADC_CONTEXT_INIT_SYNC(adc_nxp_s32_data_##n, ctx),		\
	};									\
	static const struct adc_nxp_s32_config adc_nxp_s32_config_##n = {	\
		.base = (ADC_Type *)DT_INST_REG_ADDR(n),			\
		.instance = ADC_NXP_S32_GET_INSTANCE(n),			\
		.group_channel = DT_INST_ENUM_IDX(n, group_channel),		\
		.callback_select = DT_INST_ENUM_IDX(n, callback_select),	\
		.adc_cfg = (Adc_Sar_Ip_ConfigType *)&adc_nxp_s32_default_config##n,\
		.irq_config_func = adc_nxp_s32_adc_sar_config_func_##n,		\
		.pin_cfg = COND_CODE_1(DT_INST_NUM_PINCTRL_STATES(n),		\
				(PINCTRL_DT_INST_DEV_CONFIG_GET(n)), (NULL)),	\
	};									\
	DEVICE_DT_INST_DEFINE(n,						\
			&adc_nxp_s32_init,					\
			NULL,							\
			&adc_nxp_s32_data_##n,					\
			&adc_nxp_s32_config_##n,				\
			POST_KERNEL,						\
			CONFIG_ADC_INIT_PRIORITY,				\
			&adc_nxp_s32_driver_api_##n);

DT_INST_FOREACH_STATUS_OKAY(ADC_NXP_S32_INIT_DEVICE)
