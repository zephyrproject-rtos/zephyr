/*
 * Copyright (c) 2022 Andriy Gelman, andriy.gelman@gmail.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_xmc4xxx_adc

#include <errno.h>
#include <soc.h>
#include <stdint.h>
#include <xmc_scu.h>
#include <xmc_vadc.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#define LOG_LEVEL CONFIG_ADC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adc_xmc4xxx);

#define XMC4XXX_CHANNEL_COUNT 8

struct adc_xmc4xxx_data {
	struct adc_context ctx;
	const struct device *dev;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
	uint8_t channel_mask;
};

struct adc_xmc4xxx_cfg {
	XMC_VADC_GROUP_t *base;
	void (*irq_cfg_func)(void);
	uint8_t irq_num;
};

static bool adc_global_init;
static XMC_VADC_GLOBAL_t *const adc_global_ptr = (XMC_VADC_GLOBAL_t *)0x40004000;

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_xmc4xxx_data *data = CONTAINER_OF(ctx, struct adc_xmc4xxx_data, ctx);
	const struct device *dev = data->dev;
	const struct adc_xmc4xxx_cfg *config = dev->config;
	VADC_G_TypeDef *adc_group = config->base;

	data->repeat_buffer = data->buffer;

	XMC_VADC_GROUP_ScanTriggerConversion(adc_group);

	XMC_VADC_GROUP_ScanEnableArbitrationSlot(adc_group);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx,
					      bool repeat_sampling)
{
	struct adc_xmc4xxx_data *data = CONTAINER_OF(ctx, struct adc_xmc4xxx_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static void adc_xmc4xxx_isr(const struct device *dev)
{
	struct adc_xmc4xxx_data *data = dev->data;
	const struct adc_xmc4xxx_cfg *config = dev->config;
	XMC_VADC_GROUP_t *adc_group = config->base;
	uint32_t channel_mask = data->channel_mask;
	uint32_t ch;

	/* Conversion has completed. */
	while (channel_mask > 0) {
		ch = find_lsb_set(channel_mask) - 1;
		*data->buffer++ = XMC_VADC_GROUP_GetResult(adc_group, ch);
		channel_mask &= ~BIT(ch);
	}

	adc_context_on_sampling_done(&data->ctx, dev);
	LOG_DBG("%s ISR triggered.", dev->name);
}

static int adc_xmc4xxx_validate_buffer_size(const struct adc_sequence *sequence)
{
	int active_channels = 0;
	int total_buffer_size;

	for (int i = 0; i < XMC4XXX_CHANNEL_COUNT; i++) {
		if (sequence->channels & BIT(i)) {
			active_channels++;
		}
	}

	total_buffer_size = active_channels * sizeof(uint16_t);

	if (sequence->options) {
		total_buffer_size *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < total_buffer_size) {
		return -ENOMEM;
	}

	return 0;
}

static int start_read(const struct device *dev,
		      const struct adc_sequence *sequence)
{
	int ret;
	struct adc_xmc4xxx_data *data = dev->data;
	const struct adc_xmc4xxx_cfg *config = dev->config;
	XMC_VADC_GROUP_t *adc_group = config->base;
	uint32_t requested_channels = sequence->channels;
	uint8_t resolution = sequence->resolution;
	uint32_t configured_channels = adc_group->ASSEL & requested_channels;
	XMC_VADC_GROUP_CLASS_t group_class = {0};

	if (requested_channels == 0) {
		LOG_ERR("No channels requested");
		return -EINVAL;
	}

	if (requested_channels != configured_channels) {
		LOG_ERR("Selected channels not configured");
		return -EINVAL;
	}

	if (sequence->oversampling) {
		LOG_ERR("Oversampling not supported");
		return -ENOTSUP;
	}

	ret = adc_xmc4xxx_validate_buffer_size(sequence);
	if (ret < 0) {
		LOG_ERR("Invalid sequence buffer size");
		return ret;
	}

	if (resolution == 8) {
		group_class.conversion_mode_standard = XMC_VADC_CONVMODE_8BIT;
	} else if (resolution == 10) {
		group_class.conversion_mode_standard = XMC_VADC_CONVMODE_10BIT;
	} else if (resolution == 12) {
		group_class.conversion_mode_standard = XMC_VADC_CONVMODE_12BIT;
	} else {
		LOG_ERR("Invalid resolution");
		return -EINVAL;
	}
	XMC_VADC_GROUP_InputClassInit(adc_group,  group_class, XMC_VADC_GROUP_CONV_STD, 0);

	data->channel_mask = requested_channels;
	data->buffer = sequence->buffer;
	adc_context_start_read(&data->ctx, sequence);

	return adc_context_wait_for_completion(&data->ctx);
}

static int adc_xmc4xxx_read(const struct device *dev,
			  const struct adc_sequence *sequence)
{
	int ret;
	struct adc_xmc4xxx_data *data = dev->data;

	adc_context_lock(&data->ctx, false, NULL);
	ret = start_read(dev, sequence);
	adc_context_release(&data->ctx, ret);
	return ret;
}

#ifdef CONFIG_ADC_ASYNC
static int adc_xmc4xxx_read_async(const struct device *dev,
			       const struct adc_sequence *sequence,
			       struct k_poll_signal *async)
{
	int ret;
	struct adc_xmc4xxx_data *data = dev->data;

	adc_context_lock(&data->ctx, true, async);
	ret = start_read(dev, sequence);
	adc_context_release(&data->ctx, ret);

	return ret;
}
#endif

static int adc_xmc4xxx_channel_setup(const struct device *dev,
				   const struct adc_channel_cfg *channel_cfg)
{
	const struct adc_xmc4xxx_cfg *config = dev->config;
	VADC_G_TypeDef *adc_group = config->base;
	uint32_t ch_num = channel_cfg->channel_id;
	XMC_VADC_CHANNEL_CONFIG_t channel_config = {0};

	if (ch_num >= XMC4XXX_CHANNEL_COUNT) {
		LOG_ERR("Channel %d is not valid", ch_num);
		return -EINVAL;
	}

	if (channel_cfg->differential) {
		LOG_ERR("Differential channels are not supported");
		return -EINVAL;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("Invalid channel gain");
		return -EINVAL;
	}

	if (channel_cfg->reference != ADC_REF_INTERNAL) {
		LOG_ERR("Invalid channel reference");
		return -EINVAL;
	}

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("Invalid acquisition time");
		return -EINVAL;
	}

	/* check that the group global calibration has successfully finished */
	if (adc_group->ARBCFG & VADC_G_ARBCFG_CAL_Msk) {
		LOG_WRN("Group calibration hasn't completed yet");
		return -EBUSY;
	}

	channel_config.channel_priority = true;
	channel_config.result_reg_number = ch_num;
	channel_config.result_alignment = XMC_VADC_RESULT_ALIGN_RIGHT;
	channel_config.alias_channel = -1; /* do not alias channel */
	XMC_VADC_GROUP_ChannelInit(adc_group, ch_num, &channel_config);

	adc_group->RCR[ch_num] = 0;

	XMC_VADC_GROUP_ScanAddChannelToSequence(adc_group, ch_num);

	return 0;
}

#define VADC_IRQ_MIN  18
#define IRQS_PER_VADC_GROUP 4

static int adc_xmc4xxx_init(const struct device *dev)
{
	struct adc_xmc4xxx_data *data = dev->data;
	const struct adc_xmc4xxx_cfg *config = dev->config;
	VADC_G_TypeDef *adc_group = config->base;
	uint8_t service_request;

	data->dev = dev;
	config->irq_cfg_func();

	if (adc_global_init == 0) {

		/* defined using xmc_device.h */
#ifdef CLOCK_GATING_SUPPORTED
		XMC_SCU_CLOCK_UngatePeripheralClock(XMC_SCU_PERIPHERAL_CLOCK_VADC);
#endif
		/* Reset the Hardware */
		XMC_SCU_RESET_DeassertPeripheralReset(XMC_SCU_PERIPHERAL_RESET_VADC);

		/* enable the module clock */
		adc_global_ptr->CLC = 0;

		/* global configuration register - defines clock divider to adc clock */
		/* automatic post calibration after each conversion is enabled */
		adc_global_ptr->GLOBCFG = 0;

		/* global result control register is unused */
		adc_global_ptr->GLOBRCR = 0;
		/* global bound register is unused */
		adc_global_ptr->GLOBBOUND = 0;

		adc_global_init = 1;
	}

	adc_group->ARBCFG = 0;
	adc_group->BOUND = 0;

	XMC_VADC_GROUP_SetPowerMode(adc_group, XMC_VADC_GROUP_POWERMODE_NORMAL);

	/* Initiate calibration. It is initialized for all groups. Check that the */
	/* calibration completed in the channel setup. */
	adc_global_ptr->GLOBCFG |= VADC_GLOBCFG_SUCAL_Msk;

	XMC_VADC_GROUP_BackgroundDisableArbitrationSlot(adc_group);
	XMC_VADC_GROUP_ScanDisableArbitrationSlot(adc_group);

	service_request = (config->irq_num - VADC_IRQ_MIN) % IRQS_PER_VADC_GROUP;

	XMC_VADC_GROUP_ScanSetGatingMode(adc_group, XMC_VADC_GATEMODE_IGNORE);
	XMC_VADC_GROUP_ScanSetReqSrcEventInterruptNode(adc_group, service_request);
	XMC_VADC_GROUP_ScanEnableEvent(adc_group);

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(adc, api_xmc4xxx_driver_api) = {
	.channel_setup = adc_xmc4xxx_channel_setup,
	.read = adc_xmc4xxx_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = adc_xmc4xxx_read_async,
#endif
	.ref_internal = DT_INST_PROP(0, vref_internal_mv),
};

#define ADC_XMC4XXX_CONFIG(index)						\
static void adc_xmc4xxx_cfg_func_##index(void)					\
{										\
	IRQ_CONNECT(DT_INST_IRQN(index),					\
		    DT_INST_IRQ(index, priority),				\
		    adc_xmc4xxx_isr, DEVICE_DT_INST_GET(index), 0);		\
	irq_enable(DT_INST_IRQN(index));					\
}										\
										\
static const struct adc_xmc4xxx_cfg adc_xmc4xxx_cfg_##index = {			\
	.base = (VADC_G_TypeDef *)DT_INST_REG_ADDR(index),			\
	.irq_cfg_func = adc_xmc4xxx_cfg_func_##index,				\
	.irq_num = DT_INST_IRQN(index),						\
};

#define ADC_XMC4XXX_INIT(index)							\
ADC_XMC4XXX_CONFIG(index)							\
										\
static struct adc_xmc4xxx_data adc_xmc4xxx_data_##index = {			\
	ADC_CONTEXT_INIT_TIMER(adc_xmc4xxx_data_##index, ctx),			\
	ADC_CONTEXT_INIT_LOCK(adc_xmc4xxx_data_##index, ctx),			\
	ADC_CONTEXT_INIT_SYNC(adc_xmc4xxx_data_##index, ctx),			\
};										\
										\
DEVICE_DT_INST_DEFINE(index,							\
		    &adc_xmc4xxx_init, NULL,					\
		    &adc_xmc4xxx_data_##index, &adc_xmc4xxx_cfg_##index,	\
		    POST_KERNEL, CONFIG_ADC_INIT_PRIORITY,			\
		    &api_xmc4xxx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ADC_XMC4XXX_INIT)
